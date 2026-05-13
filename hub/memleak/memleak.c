// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
// Copyright (c) 2023 Meta Platforms, Inc. and affiliates.
//
// Based on memleak(8) from BCC by Sasha Goldshtein and others.
// 1-Mar-2023   JP Kobryn   Created this.
#include <argp.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include "memleak.h"
#include "memleak.skel.h"
#include "trace_helpers.h"



static struct env {
	int interval;
	int nr_intervals;
	pid_t pid;
	long min_age_ns;
	int perf_max_stack_depth;
	int stack_map_max_entries;
	long page_size;
} env = {
	.interval = 5,
	.nr_intervals = -1,
	.pid = -1,
	.min_age_ns = 500,
	.perf_max_stack_depth = 127,
	.stack_map_max_entries = 10240,
	.page_size = 1,
};

struct allocation_node {
	uint64_t address;
	size_t size;
	struct allocation_node* next;
};

struct allocation {
	int64_t stack_id;
	size_t size;
	size_t count;
	struct allocation_node* allocations;
};

#define __ATTACH_UPROBE(skel, sym_name, prog_name, is_retprobe) \
	do { \
		LIBBPF_OPTS(bpf_uprobe_opts, uprobe_opts, \
				.func_name = #sym_name, \
				.retprobe = is_retprobe); \
		skel->links.prog_name = bpf_program__attach_uprobe_opts( \
				skel->progs.prog_name, \
				env.pid, \
				"libc.so.6", \
				0, \
				&uprobe_opts); \
	} while (false)

#define __CHECK_PROGRAM(skel, prog_name) \
	do { \
		if (!skel->links.prog_name) { \
			perror("no program attached for " #prog_name); \
			return -errno; \
		} \
	} while (false)

#define __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, is_retprobe) \
	do { \
		__ATTACH_UPROBE(skel, sym_name, prog_name, is_retprobe); \
		__CHECK_PROGRAM(skel, prog_name); \
	} while (false)

#define ATTACH_UPROBE(skel, sym_name, prog_name) __ATTACH_UPROBE(skel, sym_name, prog_name, false)
#define ATTACH_URETPROBE(skel, sym_name, prog_name) __ATTACH_UPROBE(skel, sym_name, prog_name, true)

#define ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name) __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, false)
#define ATTACH_URETPROBE_CHECKED(skel, sym_name, prog_name) __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, true)

/*
 * -EFAULT in get_stackid normally means the stack-trace is not available,
 * such as getting kernel stack trace in user mode
 */
#define STACK_ID_EFAULT(stack_id)	(stack_id == -EFAULT)

#define STACK_ID_ERR(stack_id)		((stack_id < 0) && !STACK_ID_EFAULT(stack_id))

/* hash collision (-EEXIST) suggests that stack map size may be too small */
#define STACK_ID_COLLISION(stack_id)		(stack_id == -EEXIST)

static void sig_handler(int signo);

static long argp_parse_long(int key, const char *arg, struct argp_state *state);
static error_t argp_parse_arg(int key, char *arg, struct argp_state *state);
static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args);
static void print_stack_frames_by_syms_cache();
static int print_stack_frames(struct allocation *allocs, size_t nr_allocs, int stack_traces_fd);
static int alloc_size_compare(const void *a, const void *b);
static int print_outstanding_allocs(int allocs_fd, int stack_traces_fd);
static void disable_kernel_tracepoints(struct memleak_bpf *skel);
static int attach_uprobes(struct memleak_bpf *skel);

const char *argp_program_version = "memleak 0.1";
const char *argp_program_bug_address =
	"https://github.com/iovisor/bcc/tree/master/libbpf-tools";

const char argp_args_doc[] =
"Trace outstanding memory allocations\n"
"\n"
"USAGE: memleak [-h] [-p PID] [INTERVAL] [INTERVALS]\n"
"\n"
"EXAMPLES:\n"
"./memleak -p $(pidof myapp)\n"
"        Trace allocations and display memory leaks every 5 seconds\n"
"./memleak -p 1234 10 3\n"
"        Trace allocations for PID 1234, output every 10 seconds, 3 times\n"
"";

static const struct argp_option argp_options[] = {
	{"pid", 'p', "PID", 0, "process ID to trace", 0 },
	{},
};

static volatile sig_atomic_t exiting;
static volatile sig_atomic_t child_exited;

static struct sigaction sig_action = {
	.sa_handler = sig_handler
};

struct syms_cache *syms_cache;
struct ksyms *ksyms;
static void (*print_stack_frames_func)();

static uint64_t *stack;

static struct allocation *allocs;

static void print_headers()
{
	printf("Attaching to pid %d, Ctrl+C to quit.\n", env.pid);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	struct memleak_bpf *skel = NULL;

	static const struct argp argp = {
		.options = argp_options,
		.parser = argp_parse_arg,
		.doc = argp_args_doc,
	};

	// parse command line args to env settings
	if (argp_parse(&argp, argc, argv, 0, NULL, NULL)) {
		fprintf(stderr, "failed to parse args\n");

		goto cleanup;
	}

	// install signal handler
	if (sigaction(SIGINT, &sig_action, NULL) || sigaction(SIGCHLD, &sig_action, NULL)) {
		perror("failed to set up signal handling");
		ret = -errno;

		goto cleanup;
	}

	env.page_size = sysconf(_SC_PAGE_SIZE);

	if (env.pid < 0) {
		fprintf(stderr, "PID is required. Use -p <PID>\n");
		ret = 1;
		goto cleanup;
	}

	stack = calloc(env.perf_max_stack_depth, sizeof(*stack));
	if (!stack) {
		fprintf(stderr, "failed to allocate stack array\n");
		ret = -ENOMEM;

		goto cleanup;
	}



	allocs = calloc(ALLOCS_MAX_ENTRIES, sizeof(*allocs));

	if (!allocs) {
		fprintf(stderr, "failed to allocate array\n");
		ret = -ENOMEM;

		goto cleanup;
	}

	libbpf_set_print(libbpf_print_fn);

	skel = memleak_bpf__open();
	if (!skel) {
		fprintf(stderr, "failed to open bpf object\n");
		ret = 1;

		goto cleanup;
	}

	skel->rodata->min_size = 0;
	skel->rodata->max_size = -1;
	skel->rodata->page_size = env.page_size;
	skel->rodata->sample_rate = 1;
	skel->rodata->trace_all = false;
	skel->rodata->stack_flags = BPF_F_USER_STACK;
	skel->rodata->wa_missing_free = false;
	skel->rodata->combined_only = false;

	bpf_map__set_value_size(skel->maps.stack_traces,
				env.perf_max_stack_depth * sizeof(unsigned long));
	bpf_map__set_max_entries(skel->maps.stack_traces, env.stack_map_max_entries);

	disable_kernel_tracepoints(skel);

	ret = memleak_bpf__load(skel);
	if (ret) {
		fprintf(stderr, "failed to load bpf object\n");

		goto cleanup;
	}

	const int allocs_fd = bpf_map__fd(skel->maps.allocs);
	const int stack_traces_fd = bpf_map__fd(skel->maps.stack_traces);

	ret = attach_uprobes(skel);
	if (ret) {
		fprintf(stderr, "failed to attach uprobes\n");

		goto cleanup;
	}

	ret = memleak_bpf__attach(skel);
	if (ret) {
		fprintf(stderr, "failed to attach bpf program(s)\n");

		goto cleanup;
	}

	syms_cache = syms_cache__new(0);
	if (!syms_cache) {
		fprintf(stderr, "Failed to create syms_cache\n");
		ret = -ENOMEM;

		goto cleanup;
	}

	print_stack_frames_func = print_stack_frames_by_syms_cache;

	print_headers();

	while (!exiting && env.nr_intervals) {
		env.nr_intervals--;

		sleep(env.interval);

		print_outstanding_allocs(allocs_fd, stack_traces_fd);
	}

cleanup:
if (syms_cache)
		syms_cache__free(syms_cache);
	if (ksyms)
		ksyms__free(ksyms);
	memleak_bpf__destroy(skel);

	free(allocs);
	free(stack);

	return ret;
}

long argp_parse_long(int key, const char *arg, struct argp_state *state)
{
	errno = 0;
	const long temp = strtol(arg, NULL, 10);
	if (errno || temp <= 0) {
		fprintf(stderr, "error arg:%c %s\n", (char)key, arg);
		argp_usage(state);
	}

	return temp;
}

error_t argp_parse_arg(int key, char *arg, struct argp_state *state)
{
	static int pos_args = 0;

	switch (key) {
	case 'p':
		env.pid = argp_parse_long(key, arg, state);
		break;
	case ARGP_KEY_ARG:
		pos_args++;

		if (pos_args == 1) {
			env.interval = argp_parse_long(key, arg, state);
		}
		else if (pos_args == 2) {
			env.nr_intervals = argp_parse_long(key, arg, state);
		} else {
			fprintf(stderr, "Unrecognized positional argument: %s\n", arg);
			argp_usage(state);
		}

		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	if (level == LIBBPF_DEBUG)
		return 0;

	return vfprintf(stderr, format, args);
}

void sig_handler(int signo)
{
	if (signo == SIGCHLD)
		child_exited = 1;

	exiting = 1;
}


void print_stack_frames_by_syms_cache()
{
	const struct syms *syms = syms_cache__get_syms(syms_cache, env.pid);
	if (!syms) {
		fprintf(stderr, "Failed to get syms\n");
		return;
	}

	for (size_t i = 0; i < env.perf_max_stack_depth; ++i) {
		const uint64_t addr = stack[i];

		if (addr == 0)
			break;

		struct sym_info sinfo;
		int ret = syms__map_addr_dso(syms, addr, &sinfo);
		if (ret == 0) {
			printf("\t%zu [<%016lx>]", i, addr);
			if (sinfo.sym_name)
				printf(" %s+0x%lx", sinfo.sym_name, sinfo.sym_offset);
			printf(" [%s]\n", sinfo.dso_name);
		} else {
			printf("\t%zu [<%016lx>] <%s>\n", i, addr, "null sym");
		}
	}
}

int print_stack_frames(struct allocation *allocs, size_t nr_allocs, int stack_traces_fd)
{
	for (size_t i = 0; i < nr_allocs; ++i) {
		const struct allocation *alloc = &allocs[i];

		printf("%zu bytes in %zu allocations from stack\n", alloc->size, alloc->count);

		if (bpf_map_lookup_elem(stack_traces_fd, &alloc->stack_id, stack)) {
			if (errno == ENOENT)
				continue;

			perror("failed to lookup stack trace");

			return -errno;
		}

		(*print_stack_frames_func)();
	}

	return 0;
}

int alloc_size_compare(const void *a, const void *b)
{
	const struct allocation *x = (struct allocation *)a;
	const struct allocation *y = (struct allocation *)b;

	// descending order

	if (x->size > y->size)
		return -1;

	if (x->size < y->size)
		return 1;

	return 0;
}

int print_outstanding_allocs(int allocs_fd, int stack_traces_fd)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	size_t nr_allocs = 0;
	size_t nr_missing_stacks = 0;
	size_t nr_collision_stacks = 0;

	// for each struct alloc_info "alloc_info" in the bpf map "allocs"
	for (uint64_t prev_key = 0, curr_key = 0;; prev_key = curr_key) {
		struct alloc_info alloc_info = {};
		memset(&alloc_info, 0, sizeof(alloc_info));

		if (bpf_map_get_next_key(allocs_fd, &prev_key, &curr_key)) {
			if (errno == ENOENT) {
				break; // no more keys, done
			}
			perror("map get next key error");

			return -errno;
		}

		if (bpf_map_lookup_elem(allocs_fd, &curr_key, &alloc_info)) {
			if (errno == ENOENT)
				continue;
			perror("map lookup error");

			return -errno;
		}

		// filter by age
		if (get_ktime_ns() - env.min_age_ns < alloc_info.timestamp_ns) {
			continue;
		}

		// filter invalid stacks
		if (alloc_info.stack_id < 0) {
			/* handle stack id errors */
			nr_missing_stacks += STACK_ID_ERR(alloc_info.stack_id);
			nr_collision_stacks += STACK_ID_COLLISION(alloc_info.stack_id);

			continue;
		}

		// when the stack_id exists in the allocs array,
		//   increment size with alloc_info.size
		bool stack_exists = false;

		for (size_t i = 0; !stack_exists && i < nr_allocs; ++i) {
			struct allocation *alloc = &allocs[i];

			if (alloc->stack_id == alloc_info.stack_id) {
				alloc->size += alloc_info.size;
				alloc->count++;

				stack_exists = true;
				break;
			}
		}

		if (stack_exists)
			continue;

		// when the stack_id does not exist in the allocs array,
		//   create a new entry in the array
		struct allocation alloc = {
			.stack_id = alloc_info.stack_id,
			.size = alloc_info.size,
			.count = 1,
			.allocations = NULL
		};

		memcpy(&allocs[nr_allocs], &alloc, sizeof(alloc));
		nr_allocs++;
	}

	// sort the allocs array in descending order
	qsort(allocs, nr_allocs, sizeof(allocs[0]), alloc_size_compare);

    if (nr_allocs) {
        printf("[%d:%d:%d] Top %zu stacks with outstanding allocations:\n",
                tm->tm_hour, tm->tm_min, tm->tm_sec, nr_allocs);

        print_stack_frames(allocs, nr_allocs, stack_traces_fd);
    }

	for (size_t i = 0; i < nr_allocs; i++) {
		allocs[i].stack_id = 0;
	}

	if (nr_missing_stacks > 0) {
		fprintf(stderr, "WARNING: %zu stack traces could not be displayed"
			" due to memory shortage, including %zu caused by hash collisions."
			" Consider increasing --stack-storage-size.\n",
			nr_missing_stacks, nr_collision_stacks);
	}

	return 0;
}

void disable_kernel_tracepoints(struct memleak_bpf *skel)
{
	bpf_program__set_autoload(skel->progs.memleak__kmalloc, false);
	bpf_program__set_autoload(skel->progs.memleak__kmalloc_node, false);
	bpf_program__set_autoload(skel->progs.memleak__kfree, false);
	bpf_program__set_autoload(skel->progs.memleak__kmem_cache_alloc, false);
	bpf_program__set_autoload(skel->progs.memleak__kmem_cache_alloc_node, false);
	bpf_program__set_autoload(skel->progs.memleak__kmem_cache_free, false);
	bpf_program__set_autoload(skel->progs.memleak__mm_page_alloc, false);
	bpf_program__set_autoload(skel->progs.memleak__mm_page_free, false);
	bpf_program__set_autoload(skel->progs.memleak__percpu_alloc_percpu, false);
	bpf_program__set_autoload(skel->progs.memleak__percpu_free_percpu, false);
}

int attach_uprobes(struct memleak_bpf *skel)
{
	ATTACH_UPROBE_CHECKED(skel, malloc, malloc_enter);
	ATTACH_URETPROBE_CHECKED(skel, malloc, malloc_exit);

	ATTACH_UPROBE_CHECKED(skel, calloc, calloc_enter);
	ATTACH_URETPROBE_CHECKED(skel, calloc, calloc_exit);

	ATTACH_UPROBE_CHECKED(skel, realloc, realloc_enter);
	ATTACH_URETPROBE_CHECKED(skel, realloc, realloc_exit);

	ATTACH_UPROBE_CHECKED(skel, mmap, mmap_enter);
	ATTACH_URETPROBE_CHECKED(skel, mmap, mmap_exit);

	ATTACH_UPROBE_CHECKED(skel, mremap, mremap_enter);
	ATTACH_URETPROBE_CHECKED(skel, mremap, mremap_exit);

	ATTACH_UPROBE_CHECKED(skel, posix_memalign, posix_memalign_enter);
	ATTACH_URETPROBE_CHECKED(skel, posix_memalign, posix_memalign_exit);

	ATTACH_UPROBE_CHECKED(skel, memalign, memalign_enter);
	ATTACH_URETPROBE_CHECKED(skel, memalign, memalign_exit);

	ATTACH_UPROBE_CHECKED(skel, free, free_enter);
	ATTACH_UPROBE_CHECKED(skel, munmap, munmap_enter);

	ATTACH_UPROBE(skel, valloc, valloc_enter);
	ATTACH_URETPROBE(skel, valloc, valloc_exit);

	ATTACH_UPROBE(skel, pvalloc, pvalloc_enter);
	ATTACH_URETPROBE(skel, pvalloc, pvalloc_exit);

	ATTACH_UPROBE(skel, aligned_alloc, aligned_alloc_enter);
	ATTACH_URETPROBE(skel, aligned_alloc, aligned_alloc_exit);

	return 0;
}

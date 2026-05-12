/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/* Copyright (c) 2021 Hengqi Chen */

#ifndef __CORE_FIXES_BPF_H
#define __CORE_FIXES_BPF_H

#include <vmlinux.h>
#include <bpf/bpf_core_read.h>

struct trace_event_raw_kmem_alloc___x {
	const void *ptr;
	size_t bytes_alloc;
} __attribute__((preserve_access_index));

struct trace_event_raw_kmalloc___x {
	const void *ptr;
	size_t bytes_alloc;
} __attribute__((preserve_access_index));

struct trace_event_raw_kmem_cache_alloc___x {
	const void *ptr;
	size_t bytes_alloc;
} __attribute__((preserve_access_index));

struct trace_event_raw_kmem_alloc_node___x {
	const void *ptr;
	size_t bytes_alloc;
} __attribute__((preserve_access_index));

struct trace_event_raw_kfree___x {
	const void *ptr;
} __attribute__((preserve_access_index));

struct trace_event_raw_kmem_free___x {
	const void *ptr;
} __attribute__((preserve_access_index));

struct trace_event_raw_kmem_cache_free___x {
	const void *ptr;
} __attribute__((preserve_access_index));

static __always_inline bool has_kmem_alloc(void)
{
	return false;
}

static __always_inline bool has_kmem_alloc_node(void)
{
	return false;
}

static __always_inline bool has_kfree(void)
{
	return true;
}

static __always_inline bool has_kmem_cache_free(void)
{
	return true;
}

struct cfs_rq___o {
	unsigned int nr_running;
} __attribute__((preserve_access_index));

struct cfs_rq___x {
	unsigned int nr_queued;
} __attribute__((preserve_access_index));

static __always_inline __u64 get_cfs_rq_nr_queued(void *cfs_rq)
{
	return BPF_CORE_READ((struct cfs_rq___o *)cfs_rq, nr_running);
}

struct request_queue___x {
	struct gendisk *disk;
} __attribute__((preserve_access_index));

struct request___x {
	struct request_queue___x *q;
	struct gendisk *rq_disk;
} __attribute__((preserve_access_index));

static __always_inline struct gendisk *get_disk(void *request)
{
	struct request___x *r = request;
	return BPF_CORE_READ(r, q, disk);
}

#endif /* __CORE_FIXES_BPF_H */
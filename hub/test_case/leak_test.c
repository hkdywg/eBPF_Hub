/*
 * leak_test.c - C language memory leak test program
 *
 * This program demonstrates various types of memory leaks:
 * 1. Simple malloc without free
 * 2. Loop allocation without release
 * 3. realloc failure handling
 * 4. strdup without free
 * 5. Partial release (only free first element)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define ALLOC_SIZE 1024
#define LOOP_COUNT 100
#define SLEEP_INTERVAL 1

static volatile int running = 1;

void simple_leak(void)
{
	void *ptr = malloc(ALLOC_SIZE);
	if (!ptr) {
		perror("malloc failed");
		return;
	}
	memset(ptr, 0, ALLOC_SIZE);
	printf("[simple_leak] allocated %d bytes at %p (never freed)\n", ALLOC_SIZE, ptr);
}

void loop_leak(void)
{
	for (int i = 0; i < 10; i++) {
		void *ptr = malloc(ALLOC_SIZE * 2);
		if (!ptr) {
			perror("malloc failed");
			continue;
		}
		memset(ptr, 0, ALLOC_SIZE * 2);
		printf("[loop_leak] iteration %d: allocated %d bytes at %p\n", 
		       i, ALLOC_SIZE * 2, ptr);
	}
}

void realloc_leak(void)
{
	void *ptr = malloc(ALLOC_SIZE);
	if (!ptr) {
		perror("malloc failed");
		return;
	}
	memset(ptr, 0, ALLOC_SIZE);

	void *new_ptr = realloc(ptr, ALLOC_SIZE * 4);
	if (!new_ptr) {
		printf("[realloc_leak] realloc failed, original ptr at %p leaked\n", ptr);
		return;
	}
	memset(new_ptr, 0, ALLOC_SIZE * 4);
	printf("[realloc_leak] reallocated to %d bytes at %p\n", ALLOC_SIZE * 4, new_ptr);
}

void strdup_leak(void)
{
	const char *str = "This is a test string for strdup leak";
	char *copy = strdup(str);
	if (!copy) {
		perror("strdup failed");
		return;
	}
	printf("[strdup_leak] duplicated string '%s' at %p (never freed)\n", copy, copy);
}

void partial_leak(void)
{
	void *ptrs[5];
	
	for (int i = 0; i < 5; i++) {
		ptrs[i] = malloc(ALLOC_SIZE);
		if (!ptrs[i]) {
			perror("malloc failed");
			continue;
		}
		memset(ptrs[i], 0, ALLOC_SIZE);
		printf("[partial_leak] allocated element %d at %p\n", i, ptrs[i]);
	}

	free(ptrs[0]);
	printf("[partial_leak] freed only element 0, others leaked\n");
}

void array_leak(void)
{
	int **arr = malloc(10 * sizeof(int *));
	if (!arr) {
		perror("malloc failed");
		return;
	}

	for (int i = 0; i < 10; i++) {
		arr[i] = malloc(100 * sizeof(int));
		if (!arr[i]) {
			perror("malloc failed");
			continue;
		}
		for (int j = 0; j < 100; j++) {
			arr[i][j] = i * 100 + j;
		}
		printf("[array_leak] allocated row %d at %p\n", i, arr[i]);
	}

	free(arr);
	printf("[array_leak] freed array pointer but not individual rows\n");
}

void mmap_leak(void)
{
	void *ptr = malloc(ALLOC_SIZE * 100);
	if (!ptr) {
		perror("malloc failed");
		return;
	}
	memset(ptr, 0, ALLOC_SIZE * 100);
	printf("[mmap_leak] allocated large block %d bytes at %p (never freed)\n",
	       ALLOC_SIZE * 100, ptr);
}

void calloc_leak(void)
{
	void *ptr = calloc(50, sizeof(int));
	if (!ptr) {
		perror("calloc failed");
		return;
	}
	printf("[calloc_leak] allocated %d ints (%zu bytes) at %p (never freed)\n",
	       50, 50 * sizeof(int), ptr);
}

void nested_leak(void)
{
	struct node {
		int value;
		struct node *next;
	};

	struct node *head = malloc(sizeof(struct node));
	if (!head) {
		perror("malloc failed");
		return;
	}
	head->value = 1;
	head->next = NULL;

	struct node *current = head;
	for (int i = 2; i <= 5; i++) {
		current->next = malloc(sizeof(struct node));
		if (!current->next) {
			perror("malloc failed");
			break;
		}
		current->next->value = i;
		current->next->next = NULL;
		current = current->next;
		printf("[nested_leak] created node %d at %p\n", i, current);
	}

	free(head);
	printf("[nested_leak] freed head only, rest of list leaked\n");
}

int main(int argc, char *argv[])
{
	printf("=== Memory Leak Test Program (C Version) ===\n");
	printf("Run with: sudo ./build_output/hub/memleak/memleak -p $(pidof leak_test)\n");
	printf("Press Ctrl+C to stop\n\n");

	int iterations = 0;

	while (running) {
		printf("\n--- Iteration %d ---\n", iterations + 1);

		simple_leak();
#if 0
		loop_leak();
		realloc_leak();
		strdup_leak();
		partial_leak();
		array_leak();
		mmap_leak();
		calloc_leak();
		nested_leak();
#endif
		sleep(SLEEP_INTERVAL);
		iterations++;

		if (iterations >= LOOP_COUNT) {
			printf("\nReached %d iterations, continuing to leak...\n", LOOP_COUNT);
		}
	}

	printf("\nProgram terminated (all memory intentionally leaked)\n");
	return 0;
}

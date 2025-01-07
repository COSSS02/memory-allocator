// SPDX-License-Identifier: BSD-3-Clause

#include "osmem_utils.h"

struct block_meta sentinel = { 0, STATUS_FREE, NULL, NULL };

int block_meta_wasnt_preallocated(void)
{
	return sentinel.status == STATUS_FREE;
}

void block_meta_add(void *address, size_t size, int status)
{
	struct block_meta *current = &sentinel;

	while (current->next)
		current = current->next;

	current->next = address;
	current->next->size = size;
	current->next->status = status;
	current->next->prev = current;
	current->next->next = NULL;

	if (block_meta_wasnt_preallocated() && status == STATUS_ALLOC) {
		split(current->next, size, MMAP_THRESHOLD);
		sentinel.status = STATUS_ALLOC;
	}
}

struct block_meta *block_meta_find(void *address)
{
	struct block_meta *current = sentinel.next;

	while (current != address)
		current = current->next;

	return current;
}

struct block_meta *find_best_block(size_t size)
{
	struct block_meta *current = sentinel.next;
	struct block_meta *best = NULL;
	struct block_meta *expand = NULL;

	if (current)
		coalesce();

	while (current) {
		if (current->status == STATUS_FREE) {
			if (size <= current->size &&
				(best == NULL || best->size > current->size))
				best = current;

			expand = current;

		} else if (current->status == STATUS_ALLOC) {
			expand = NULL;
		}

		current = current->next;
	}

	if (best) {
		best->status = STATUS_ALLOC;
		split(best, size, best->size);

	} else if (expand) {
		void *address = (void *) expand;

		int ret = brk(address + ALIGN_SIZE(size) + METADATA_SIZE);

		DIE(ret == -1, "brk failed when expanding last block");

		best = expand;
		best->size = size;
		best->status = STATUS_ALLOC;
	}

	return best;
}

void split(struct block_meta *address, size_t size, size_t total_size)
{
	size_t alloc_size = ALIGN_SIZE(size) + METADATA_SIZE;

	if (block_meta_wasnt_preallocated())
		alloc_size += METADATA_SIZE;

	if (total_size >= alloc_size + ALIGN) {
		struct block_meta *temp = address->next;

		address->size = size;
		address->next = (struct block_meta *) ((char *) address + alloc_size);
		address->next->size = total_size - alloc_size;
		address->next->status = STATUS_FREE;
		address->next->prev = address;
		address->next->next = temp;

		if (temp)
			address->next->next->prev = address->next;
	}
}

void coalesce(void)
{
	struct block_meta *current = sentinel.next;

	while (current->next) {
		if (current->status == STATUS_FREE &&
			current->next->status == STATUS_FREE) {
			current->size += ALIGN_SIZE(current->next->size) + METADATA_SIZE;
			current->size += ALIGN_SIZE(current->size) - current->size;

			if (current->next->next)
				current->next->next->prev = current;

			current->next = current->next->next;

			current = current->prev;
		}
		current = current->next;
	}
}

struct block_meta *find_best_block_realloc(struct block_meta *address,
	size_t size)
{
	if (!address->next) {
		int ret = brk((char *) address + ALIGN_SIZE(size) + METADATA_SIZE);

		DIE(ret == -1, "brk failed when expanding last block");

		address->size = size;

		return address;
	}

	if (address->next->status == STATUS_FREE) {
		coalesce();

		if (ALIGN_SIZE(address->size) + METADATA_SIZE +
				ALIGN_SIZE(address->next->size) >= size) {
			address->size += address->next->size + METADATA_SIZE;
			address->next = address->next->next;

			split(address, size, address->size);

			return address;
		}
	}

	return NULL;
}

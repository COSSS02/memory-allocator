// SPDX-License-Identifier: BSD-3-Clause

#include <sys/mman.h>
#include <string.h>
#include "osmem.h"
#include "osmem_utils.h"

void *os_malloc(size_t size)
{
	/* TODO: Implement os_malloc */
	if (!size)
		return NULL;

	void *address;
	size_t alloc_size = METADATA_SIZE + ALIGN_SIZE(size);

	if (alloc_size < MMAP_THRESHOLD) {
		if (block_meta_wasnt_preallocated()) {
			address = sbrk(MMAP_THRESHOLD);

			DIE(address == MAP_FAILED, "sbrk preallocation failed");

			block_meta_add(address, size, STATUS_ALLOC);

		} else {
			address = find_best_block(size);

			if (!address) {
				address = sbrk(alloc_size);

				DIE(address == MAP_FAILED, "sbrk allocation failed");

				block_meta_add(address, size, STATUS_ALLOC);
			}
		}

	} else {
		address = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		DIE(address == MAP_FAILED, "mmap failed");

		block_meta_add(address, size, STATUS_MAPPED);
	}

	return address + METADATA_SIZE;
}

void os_free(void *ptr)
{
	/* TODO: Implement os_free */
	if (ptr != NULL) {
		struct block_meta *ptr_free;

		ptr_free = block_meta_find(ptr - METADATA_SIZE);

		if (ptr_free->status == STATUS_MAPPED) {
			int ret;

			ptr_free->prev->next = ptr_free->next;
			if (ptr_free->next)
				ptr_free->next->prev = ptr_free->prev;

			ret = munmap(ptr_free, ALIGN_SIZE(ptr_free->size) + METADATA_SIZE);

			DIE(ret, "munmap failed");

		} else {
			ptr_free->status = STATUS_FREE;
		}
	}
}

void *os_calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement os_calloc */
	if (nmemb == 0 || size == 0)
		return NULL;

	void *address;
	size_t alloc_size = METADATA_SIZE + ALIGN_SIZE(nmemb * size);

	if (alloc_size < (size_t) getpagesize()) {
		if (block_meta_wasnt_preallocated()) {
			address = sbrk(MMAP_THRESHOLD);

			DIE(address == MAP_FAILED, "sbrk preallocation failed");

			block_meta_add(address, nmemb * size, STATUS_ALLOC);

		} else {
			address = find_best_block(nmemb * size);

			if (!address) {
				address = sbrk(alloc_size);

				DIE(address == MAP_FAILED, "sbrk allocation failed");

				block_meta_add(address, nmemb * size, STATUS_ALLOC);
			}
		}

	} else {
		address = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		DIE(address == MAP_FAILED, "mmap failed");

		block_meta_add(address, nmemb * size, STATUS_MAPPED);
	}

	memset(address + METADATA_SIZE, 0, ALIGN_SIZE(nmemb * size));

	return address + METADATA_SIZE;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	if (!ptr)
		return os_malloc(size);

	if (!size) {
		os_free(ptr);
		return NULL;
	}

	struct block_meta *ptr_realloc;
	void *address = NULL;

	ptr_realloc = block_meta_find(ptr - METADATA_SIZE);

	if (ptr_realloc->status == STATUS_FREE)
		return NULL;

	if (ptr_realloc->status == STATUS_ALLOC) {
		if (size <= ptr_realloc->size) {
			if (ptr_realloc->next) {
				size_t total_size;

				total_size = (char *) ptr_realloc->next - (char *) ptr_realloc;
				total_size -= METADATA_SIZE;

				split(ptr_realloc, size, total_size);
			}

			return ptr;
		}

		address = find_best_block_realloc(ptr_realloc, size);
	}

	if (address) {
		address += METADATA_SIZE;

	} else {
		address = os_malloc(size);

		memcpy(address, ptr, MIN(size, ptr_realloc->size));

		os_free(ptr);
	}

	return address;
}

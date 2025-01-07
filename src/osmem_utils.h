#ifndef _OSMEM_UTILS_H_
#define _OSMEM_UTILS_H_

#include <unistd.h>
#include "block_meta.h"

#define ALIGN 8
#define ALIGN_SIZE(size) (((size) / (ALIGN) * (ALIGN)) + \
						  (((size) % (ALIGN)) ? (ALIGN) : (0)))
#define METADATA_SIZE (ALIGN_SIZE(sizeof(struct block_meta)))
#define MMAP_THRESHOLD (128 * 1024)
#define MAP_FAILED ((void *) -1)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void exit(int);

int block_meta_wasnt_preallocated(void);

void block_meta_add(void *address, size_t size, int status);

struct block_meta *block_meta_find(void *address);

struct block_meta *find_best_block(size_t size);

void split(struct block_meta *address, size_t size, size_t total_size);

void coalesce(void);

struct block_meta *find_best_block_realloc(struct block_meta *address,
	size_t size);

#endif

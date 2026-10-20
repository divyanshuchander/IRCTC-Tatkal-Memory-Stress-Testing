#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *memory_alloc(size_t size);
void memory_free(void *pointer);
void memory_fail_after(long allocation_number);
size_t memory_current_bytes(void);
size_t memory_peak_bytes(void);
size_t memory_allocations(void);
size_t memory_frees(void);
size_t memory_failures(void);
size_t memory_outstanding(void);

#endif

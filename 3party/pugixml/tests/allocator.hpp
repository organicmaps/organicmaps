#ifndef HEADER_TEST_ALLOCATOR_HPP
#define HEADER_TEST_ALLOCATOR_HPP

#include <stddef.h>

void* memory_allocate(size_t size);
size_t memory_size(void* ptr);
void memory_deallocate(void* ptr);

#endif

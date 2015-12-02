#include "allocator.hpp"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

// Address sanitizer
#if defined(__has_feature)
#	define ADDRESS_SANITIZER __has_feature(address_sanitizer)
#else
#	define ADDRESS_SANITIZER defined(__SANITIZE_ADDRESS__)
#endif

// Low-level allocation functions
#if defined(_WIN32) || defined(_WIN64)
#	ifdef __MWERKS__
#		pragma ANSI_strict off // disable ANSI strictness to include windows.h
#		pragma cpp_extensions on // enable some extensions to include windows.h
#	endif

#   if defined(_MSC_VER)
#       pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#   endif

#	ifdef _XBOX_VER
#		define NOD3D
#		include <xtl.h>
#	else
#		include <windows.h>
#	endif

namespace
{
	const size_t page_size = 4096;

	size_t align_to_page(size_t value)
	{
		return (value + page_size - 1) & ~(page_size - 1);
	}

	void* allocate_page_aligned(size_t size)
	{
		// We can't use VirtualAlloc because it has 64Kb granularity so we run out of address space quickly
		// We can't use malloc because of occasional problems with CW on CRT termination
		static HANDLE heap = HeapCreate(0, 0, 0);

		void* result = HeapAlloc(heap, 0, size + page_size);

		return reinterpret_cast<void*>(align_to_page(reinterpret_cast<size_t>(result)));
	}

	void* allocate(size_t size)
	{
		size_t aligned_size = align_to_page(size);

		void* ptr = allocate_page_aligned(aligned_size + page_size);
		if (!ptr) return 0;

		char* end = static_cast<char*>(ptr) + aligned_size;

		DWORD old_flags;
		VirtualProtect(end, page_size, PAGE_NOACCESS, &old_flags);

		return end - size;
	}

	void deallocate(void* ptr, size_t size)
	{
		size_t aligned_size = align_to_page(size);

		void* rptr = static_cast<char*>(ptr) + size - aligned_size;

		DWORD old_flags;
		VirtualProtect(rptr, aligned_size + page_size, PAGE_NOACCESS, &old_flags);
	}
}
#elif (defined(__APPLE__) || defined(__linux__)) && (defined(__i386) || defined(__x86_64)) && !ADDRESS_SANITIZER
#	include <sys/mman.h>

namespace
{
	const size_t page_size = 4096;

	size_t align_to_page(size_t value)
	{
		return (value + page_size - 1) & ~(page_size - 1);
	}

	void* allocate_page_aligned(size_t size)
	{
		void* result = malloc(size + page_size);

		return reinterpret_cast<void*>(align_to_page(reinterpret_cast<size_t>(result)));
	}

	void* allocate(size_t size)
	{
		size_t aligned_size = align_to_page(size);

		void* ptr = allocate_page_aligned(aligned_size + page_size);
		if (!ptr) return 0;

		char* end = static_cast<char*>(ptr) + aligned_size;

		int res = mprotect(end, page_size, PROT_NONE);
		assert(res == 0);
		(void)!res;

		return end - size;
	}

	void deallocate(void* ptr, size_t size)
	{
		size_t aligned_size = align_to_page(size);

		void* rptr = static_cast<char*>(ptr) + size - aligned_size;

		int res = mprotect(rptr, aligned_size + page_size, PROT_NONE);
		assert(res == 0);
		(void)!res;
	}
}
#else
namespace
{
	void* allocate(size_t size)
	{
		return malloc(size);
	}

	void deallocate(void* ptr, size_t size)
	{
		(void)size;

		free(ptr);
	}
}
#endif

// High-level allocation functions
const size_t memory_alignment = sizeof(double) > sizeof(void*) ? sizeof(double) : sizeof(void*);

void* memory_allocate(size_t size)
{
	void* result = allocate(size + memory_alignment);
	if (!result) return 0;

	memcpy(result, &size, sizeof(size_t));

	return static_cast<char*>(result) + memory_alignment;
}

size_t memory_size(void* ptr)
{
	assert(ptr);

	size_t result;
	memcpy(&result, static_cast<char*>(ptr) - memory_alignment, sizeof(size_t));

	return result;
}

void memory_deallocate(void* ptr)
{
	if (!ptr) return;

	size_t size = memory_size(ptr);

	deallocate(static_cast<char*>(ptr) - memory_alignment, size + memory_alignment);
}


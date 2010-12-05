/*
** Copyright (c) 2001-2007 Expat maintainers.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>

void * malloc (size_t len)
{
	uint32 size = sizeof(uint32) + len;

	uint32 *mem = AllocMem(size, MEMF_SHARED);
	if ( mem != 0 )  {
		*mem = size;
		++mem;
	}

	return mem;
}


void * realloc (void * mem, size_t len2)
{
	if ( mem == 0 )  {
		return malloc(len2);
	}

	if ( len2 == 0 )  {
		free(mem);
		return 0;
	}

	void * new_mem = malloc(len2);
	if ( new_mem == 0 )  {
		return 0;
	}

	uint32 mem_size = *(((uint32*)mem) - 1);
	CopyMem(mem, new_mem, mem_size);
	free(mem);

	return new_mem;
}


void free (void * mem)
{
	if ( mem != 0 )  {
		uint32 * size_ptr = ((uint32*)mem) - 1;
		FreeMem(size_ptr, *size_ptr);
	}
}


int memcmp (const void * a, const void * b, size_t len)
{
	size_t i;
	int diff;

	for ( i = 0; i < len; ++i )  {
		diff = *((uint8 *)a++) - *((uint8 *)b++);
		if ( diff )  {
			return diff;
		}
	}

	return 0;
}


void * memcpy (void * t, const void * a, size_t len)
{
	CopyMem((APTR)a, t, len);
	return t;
}


void * memmove (void * t1, const void * t2, size_t len)
{
	MoveMem((APTR)t2, t1, len);
	return t1;
}


void * memset (void * t, int c, size_t len)
{
	return SetMem(t, c, len);
}

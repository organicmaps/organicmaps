#ifndef MMAP_FOR_WINDOWS_HPP
#define MMAP_FOR_WINDOWS_HPP

/* mmap() replacement for Windows
 *
 * Author: Mike Frysinger <vapier@gentoo.org>
 * Placed into the public domain
 */

/* References:
 * CreateFileMapping: http://msdn.microsoft.com/en-us/library/aa366537(VS.85).aspx
 * CloseHandle:       http://msdn.microsoft.com/en-us/library/ms724211(VS.85).aspx
 * MapViewOfFile:     http://msdn.microsoft.com/en-us/library/aa366761(VS.85).aspx
 * UnmapViewOfFile:   http://msdn.microsoft.com/en-us/library/aa366882(VS.85).aspx
 */

#include <io.h>
#include <windows.h>
#include <sys/types.h>

#define PROT_READ     0x1
#define PROT_WRITE    0x2
/* This flag is only available in WinXP+ */
#ifdef FILE_MAP_EXECUTE
#define PROT_EXEC     0x4
#else
#define PROT_EXEC        0x0
#define FILE_MAP_EXECUTE 0
#endif

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_FAILED    ((void *) -1)

static DWORD dword_hi(uint64_t x) {
    return static_cast<DWORD>(x >> 32);
}

static DWORD dword_lo(uint64_t x) {
    return static_cast<DWORD>(x & 0xffffffff);
}

static void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
        return MAP_FAILED;
    if (fd == -1) {
        if (!(flags & MAP_ANON) || offset)
            return MAP_FAILED;
    } else if (flags & MAP_ANON)
        return MAP_FAILED;

    DWORD flProtect;
    if (prot & PROT_WRITE) {
        if (prot & PROT_EXEC)
            flProtect = PAGE_EXECUTE_READWRITE;
        else
            flProtect = PAGE_READWRITE;
    } else if (prot & PROT_EXEC) {
        if (prot & PROT_READ)
            flProtect = PAGE_EXECUTE_READ;
        else if (prot & PROT_EXEC)
            flProtect = PAGE_EXECUTE;
    } else
        flProtect = PAGE_READONLY;

    uint64_t end = static_cast<uint64_t>(length) + offset;
    HANDLE mmap_fd;
    if (fd == -1)
        mmap_fd = INVALID_HANDLE_VALUE;
    else
        mmap_fd = (HANDLE)_get_osfhandle(fd);

    HANDLE h = CreateFileMapping(mmap_fd, NULL, flProtect, dword_hi(end), dword_lo(end), NULL);
    if (h == NULL)
        return MAP_FAILED;

    DWORD dwDesiredAccess;
    if (prot & PROT_WRITE)
        dwDesiredAccess = FILE_MAP_WRITE;
    else
        dwDesiredAccess = FILE_MAP_READ;
    if (prot & PROT_EXEC)
        dwDesiredAccess |= FILE_MAP_EXECUTE;
    if (flags & MAP_PRIVATE)
        dwDesiredAccess |= FILE_MAP_COPY;
    void *ret = MapViewOfFile(h, dwDesiredAccess, dword_hi(offset), dword_lo(offset), length);
    if (ret == NULL) {
        CloseHandle(h);
        ret = MAP_FAILED;
    }
    return ret;
}

static int munmap(void *addr, size_t length)
{
    return UnmapViewOfFile(addr) ? 0 : -1;
    /* ruh-ro, we leaked handle from CreateFileMapping() ... */
}

#endif

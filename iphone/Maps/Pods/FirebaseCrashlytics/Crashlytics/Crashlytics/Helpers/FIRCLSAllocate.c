// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdatomic.h>

#include "FIRCLSAllocate.h"
#include "FIRCLSHost.h"
#include "FIRCLSUtility.h"

#include <errno.h>
#include <libkern/OSAtomic.h>
#include <mach/vm_map.h>
#include <mach/vm_param.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

void* FIRCLSAllocatorSafeAllocateFromRegion(FIRCLSAllocationRegion* region, size_t size);

FIRCLSAllocatorRef FIRCLSAllocatorCreate(size_t writableSpace, size_t readableSpace) {
  FIRCLSAllocatorRef allocator;
  FIRCLSAllocationRegion writableRegion;
  FIRCLSAllocationRegion readableRegion;
  size_t allocationSize;
  vm_size_t pageSize;
  void* buffer;

  // | GUARD | WRITABLE_REGION | GUARD | READABLE_REGION | GUARD |

  pageSize = FIRCLSHostGetPageSize();

  readableSpace += sizeof(FIRCLSAllocator);  // add the space for our allocator itself

  // we can only protect at the page level, so we need all of our regions to be
  // exact multples of pages.  But, we don't need anything in the special-case of zero.

  writableRegion.size = 0;
  if (writableSpace > 0) {
    writableRegion.size = ((writableSpace / pageSize) + 1) * pageSize;
  }

  readableRegion.size = 0;
  if (readableSpace > 0) {
    readableRegion.size = ((readableSpace / pageSize) + 1) * pageSize;
  }

  // Make one big, continous allocation, adding additional pages for our guards.  Note
  // that we cannot use malloc (or valloc) in this case, because we need to assert full
  // ownership over these allocations.  mmap is a much better choice.  We also mark these
  // pages as MAP_NOCACHE.
  allocationSize = writableRegion.size + readableRegion.size + pageSize * 3;
  buffer =
      mmap(0, allocationSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_NOCACHE, -1, 0);
  if (buffer == MAP_FAILED) {
    FIRCLSSDKLogError("Mapping failed %s\n", strerror(errno));
    return NULL;
  }

  // move our cursors into position
  writableRegion.cursor = (void*)((uintptr_t)buffer + pageSize);
  readableRegion.cursor = (void*)((uintptr_t)buffer + pageSize + writableRegion.size + pageSize);
  writableRegion.start = writableRegion.cursor;
  readableRegion.start = readableRegion.cursor;

  FIRCLSSDKLogInfo("Mapping: %p %p %p, total: %zu K\n", buffer, writableRegion.start,
                   readableRegion.start, allocationSize / 1024);

  // protect first guard page
  if (mprotect(buffer, pageSize, PROT_NONE) != 0) {
    FIRCLSSDKLogError("First guard protection failed %s\n", strerror(errno));
    return NULL;
  }

  // middle guard
  if (mprotect((void*)((uintptr_t)buffer + pageSize + writableRegion.size), pageSize, PROT_NONE) !=
      0) {
    FIRCLSSDKLogError("Middle guard protection failed %s\n", strerror(errno));
    return NULL;
  }

  // end guard
  if (mprotect((void*)((uintptr_t)buffer + pageSize + writableRegion.size + pageSize +
                       readableRegion.size),
               pageSize, PROT_NONE) != 0) {
    FIRCLSSDKLogError("Last guard protection failed %s\n", strerror(errno));
    return NULL;
  }

  // now, perform our first "allocation", which is to place our allocator into the read-only region
  allocator = FIRCLSAllocatorSafeAllocateFromRegion(&readableRegion, sizeof(FIRCLSAllocator));

  // set up its data structure
  allocator->buffer = buffer;
  allocator->protectionEnabled = false;
  allocator->readableRegion = readableRegion;
  allocator->writeableRegion = writableRegion;

  FIRCLSSDKLogDebug("Allocator successfully created %p", allocator);

  return allocator;
}

void FIRCLSAllocatorDestroy(FIRCLSAllocatorRef allocator) {
  if (allocator) {
  }
}

bool FIRCLSAllocatorProtect(FIRCLSAllocatorRef allocator) {
  void* address;

  if (!FIRCLSIsValidPointer(allocator)) {
    FIRCLSSDKLogError("Invalid allocator");
    return false;
  }

  if (allocator->protectionEnabled) {
    FIRCLSSDKLogWarn("Write protection already enabled");
    return true;
  }

  // This has to be done first
  allocator->protectionEnabled = true;

  vm_size_t pageSize = FIRCLSHostGetPageSize();

  // readable region
  address =
      (void*)((uintptr_t)allocator->buffer + pageSize + allocator->writeableRegion.size + pageSize);

  return mprotect(address, allocator->readableRegion.size, PROT_READ) == 0;
}

bool FIRCLSAllocatorUnprotect(FIRCLSAllocatorRef allocator) {
  size_t bufferSize;

  if (!allocator) {
    return false;
  }

  vm_size_t pageSize = FIRCLSHostGetPageSize();

  bufferSize = (uintptr_t)allocator->buffer + pageSize + allocator->writeableRegion.size +
               pageSize + allocator->readableRegion.size + pageSize;

  allocator->protectionEnabled =
      !(mprotect(allocator->buffer, bufferSize, PROT_READ | PROT_WRITE) == 0);

  return allocator->protectionEnabled;
}

void* FIRCLSAllocatorSafeAllocateFromRegion(FIRCLSAllocationRegion* region, size_t size) {
  void* newCursor;
  void* originalCursor;

  // Here's the idea
  // - read the current cursor
  // - compute what our new cursor should be
  // - attempt a swap
  // if the swap fails, some other thread has modified stuff, and we have to start again
  // if the swap works, everything has been updated correctly and we are done
  do {
    originalCursor = region->cursor;

    // this shouldn't happen unless we make a mistake with our size pre-computations
    if ((uintptr_t)originalCursor - (uintptr_t)region->start + size > region->size) {
      FIRCLSSDKLog("Unable to allocate sufficient memory, falling back to malloc\n");
      void* ptr = malloc(size);
      if (!ptr) {
        FIRCLSSDKLog("Unable to malloc in FIRCLSAllocatorSafeAllocateFromRegion\n");
        return NULL;
      }
      return ptr;
    }

    newCursor = (void*)((uintptr_t)originalCursor + size);
  } while (!atomic_compare_exchange_strong(&region->cursor, &originalCursor, newCursor));

  return originalCursor;
}

void* FIRCLSAllocatorSafeAllocate(FIRCLSAllocatorRef allocator,
                                  size_t size,
                                  FIRCLSAllocationType type) {
  FIRCLSAllocationRegion* region;

  if (!allocator) {
    // fall back to malloc in this case
    FIRCLSSDKLog("Allocator invalid, falling back to malloc\n");
    void* ptr = malloc(size);
    if (!ptr) {
      FIRCLSSDKLog("Unable to malloc in FIRCLSAllocatorSafeAllocate\n");
      return NULL;
    }
    return ptr;
  }

  if (allocator->protectionEnabled) {
    FIRCLSSDKLog("Allocator already protected, falling back to malloc\n");
    void* ptr = malloc(size);
    if (!ptr) {
      FIRCLSSDKLog("Unable to malloc in FIRCLSAllocatorSafeAllocate\n");
      return NULL;
    }
    return ptr;
  }

  switch (type) {
    case CLS_READONLY:
      region = &allocator->readableRegion;
      break;
    case CLS_READWRITE:
      region = &allocator->writeableRegion;
      break;
    default:
      return NULL;
  }

  return FIRCLSAllocatorSafeAllocateFromRegion(region, size);
}

void FIRCLSAllocatorFree(FIRCLSAllocatorRef allocator, void* ptr) {
  if (!allocator) {
    free(ptr);
  }

  // how do we do deallocations?
}

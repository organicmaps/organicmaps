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

#include "FIRCLSFeatures.h"

#pragma once

#include <stdbool.h>
#include <sys/types.h>

typedef enum { CLS_READONLY = 0, CLS_READWRITE = 1 } FIRCLSAllocationType;

typedef struct {
  size_t size;
  void* start;
  _Atomic(void*) volatile cursor;
} FIRCLSAllocationRegion;

typedef struct {
  void* buffer;
  bool protectionEnabled;
  FIRCLSAllocationRegion writeableRegion;
  FIRCLSAllocationRegion readableRegion;
} FIRCLSAllocator;
typedef FIRCLSAllocator* FIRCLSAllocatorRef;

FIRCLSAllocatorRef FIRCLSAllocatorCreate(size_t writableSpace, size_t readableSpace);
void FIRCLSAllocatorDestroy(FIRCLSAllocatorRef allocator);

bool FIRCLSAllocatorProtect(FIRCLSAllocatorRef allocator);
bool FIRCLSAllocatorUnprotect(FIRCLSAllocatorRef allocator);

void* FIRCLSAllocatorSafeAllocate(FIRCLSAllocatorRef allocator,
                                  size_t size,
                                  FIRCLSAllocationType type);
const char* FIRCLSAllocatorSafeStrdup(FIRCLSAllocatorRef allocator, const char* string);
void FIRCLSAllocatorFree(FIRCLSAllocatorRef allocator, void* ptr);

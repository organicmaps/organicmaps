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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FIRCLSFeatures.h"
#include "FIRCLSFile.h"
#include "FIRCLSMachO.h"

__BEGIN_DECLS

// Typically, apps seem to have ~300 binary images loaded
#define CLS_BINARY_IMAGE_RUNTIME_NODE_COUNT (512)
#define CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE (32)
#define CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME 0

#define FIRCLSUUIDStringLength (33)

typedef struct {
  _Atomic(void*) volatile baseAddress;
  uint64_t size;
#if CLS_DWARF_UNWINDING_SUPPORTED
  const void* ehFrame;
#endif
#if CLS_COMPACT_UNWINDING_SUPPORTED
  const void* unwindInfo;
#endif
  const void* crashInfo;
#if CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME
  char name[CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE];
#endif
} FIRCLSBinaryImageRuntimeNode;

typedef struct {
  char uuidString[FIRCLSUUIDStringLength];
  bool encrypted;
  FIRCLSMachOVersion builtSDK;
  FIRCLSMachOVersion minSDK;
  FIRCLSBinaryImageRuntimeNode node;
  struct FIRCLSMachOSlice slice;
  intptr_t vmaddr_slide;
} FIRCLSBinaryImageDetails;

typedef struct {
  const char* path;
} FIRCLSBinaryImageReadOnlyContext;

typedef struct {
  FIRCLSFile file;
  FIRCLSBinaryImageRuntimeNode nodes[CLS_BINARY_IMAGE_RUNTIME_NODE_COUNT];
} FIRCLSBinaryImageReadWriteContext;

void FIRCLSBinaryImageInit(FIRCLSBinaryImageReadOnlyContext* roContext,
                           FIRCLSBinaryImageReadWriteContext* rwContext);

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSBinaryImageSafeFindImageForAddress(uintptr_t address,
                                              FIRCLSBinaryImageRuntimeNode* image);
bool FIRCLSBinaryImageSafeHasUnwindInfo(FIRCLSBinaryImageRuntimeNode* image);
#endif

bool FIRCLSBinaryImageFindImageForUUID(const char* uuidString,
                                       FIRCLSBinaryImageDetails* imageDetails);

bool FIRCLSBinaryImageRecordMainExecutable(FIRCLSFile* file);

__END_DECLS

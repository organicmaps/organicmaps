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
#include "FIRCLSThreadState.h"

// We have to pack the arrays defined in this header, so
// we can reason about pointer math.
#pragma pack(push)
#pragma pack(1)
#include <mach-o/compact_unwind_encoding.h>
#pragma pack(pop)

// First masks out the value, and then shifts the value by the number
// of zeros in the mask. __builtin_ctz returns the number of trailing zeros.
// Its output is undefined if the input is zero.
#define GET_BITS_WITH_MASK(value, mask) ((value & mask) >> (mask == 0 ? 0 : __builtin_ctz(mask)))

typedef struct {
  const void* unwindInfo;
  const void* ehFrame;
  uintptr_t loadAddress;

  struct unwind_info_section_header unwindHeader;
  struct unwind_info_section_header_index_entry indexHeader;
  uint32_t firstLevelNextFunctionOffset;
} FIRCLSCompactUnwindContext;

typedef struct {
  compact_unwind_encoding_t encoding;
  uintptr_t functionStart;
  uintptr_t functionEnd;
  uintptr_t lsda;
  uintptr_t personality;

} FIRCLSCompactUnwindResult;

bool FIRCLSCompactUnwindInit(FIRCLSCompactUnwindContext* context,
                             const void* unwindInfo,
                             const void* ehFrame,
                             uintptr_t loadAddress);
void* FIRCLSCompactUnwindGetIndexData(FIRCLSCompactUnwindContext* context);
void* FIRCLSCompactUnwindGetSecondLevelData(FIRCLSCompactUnwindContext* context);
bool FIRCLSCompactUnwindFindFirstLevelIndex(FIRCLSCompactUnwindContext* context,
                                            uintptr_t pc,
                                            uint32_t* index);

bool FIRCLSCompactUnwindDwarfFrame(FIRCLSCompactUnwindContext* context,
                                   uintptr_t dwarfOffset,
                                   FIRCLSThreadContext* registers);
bool FIRCLSCompactUnwindLookupAndCompute(FIRCLSCompactUnwindContext* context,
                                         FIRCLSThreadContext* registers);

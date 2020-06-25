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

#include "FIRCLSThreadState.h"
#include "FIRCLSUtility.h"
#if CLS_COMPACT_UNWINDING_SUPPORTED
#include "FIRCLSCompactUnwind.h"
#endif
#include <mach/vm_types.h>
#include <stdbool.h>

#include "FIRCLSUnwind_arch.h"

extern const uint32_t FIRCLSUnwindMaxFrames;

extern const uint32_t FIRCLSUnwindInfiniteRecursionCountThreshold;

typedef struct {
  FIRCLSThreadContext registers;
  uint32_t frameCount;
#if CLS_COMPACT_UNWINDING_SUPPORTED
  FIRCLSCompactUnwindContext compactUnwindState;
#endif
  uintptr_t lastFramePC;
  uint32_t repeatCount;
} FIRCLSUnwindContext;

// API
bool FIRCLSUnwindInit(FIRCLSUnwindContext* context, FIRCLSThreadContext threadContext);

bool FIRCLSUnwindNextFrame(FIRCLSUnwindContext* context);
uintptr_t FIRCLSUnwindGetPC(FIRCLSUnwindContext* context);
uintptr_t FIRCLSUnwindGetStackPointer(FIRCLSUnwindContext* context);
uint32_t FIRCLSUnwindGetFrameRepeatCount(FIRCLSUnwindContext* context);

// utility functions
bool FIRCLSUnwindIsAddressExecutable(vm_address_t address);
bool FIRCLSUnwindFirstExecutableAddress(vm_address_t start,
                                        vm_address_t end,
                                        vm_address_t* foundAddress);

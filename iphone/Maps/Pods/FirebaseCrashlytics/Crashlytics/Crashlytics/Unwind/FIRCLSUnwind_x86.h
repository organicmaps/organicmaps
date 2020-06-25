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

#include "FIRCLSCompactUnwind.h"
#include "FIRCLSFeatures.h"

// Add some abstraction to compact unwinding, because compact
// unwinding is nearly identical between 32 and 64 bit
#if CLS_CPU_X86_64

#define CLS_X86_MODE_MASK UNWIND_X86_64_MODE_MASK
#define CLS_X86_MODE_BP_FRAME UNWIND_X86_64_MODE_RBP_FRAME
#define CLS_X86_MODE_STACK_IMMD UNWIND_X86_64_MODE_STACK_IMMD
#define CLS_X86_MODE_STACK_IND UNWIND_X86_64_MODE_STACK_IND
#define CLS_X86_MODE_DWARF UNWIND_X86_64_MODE_DWARF

#define CLS_X86_BP_FRAME_REGISTERS UNWIND_X86_64_RBP_FRAME_REGISTERS
#define CLS_X86_BP_FRAME_OFFSET UNWIND_X86_64_RBP_FRAME_OFFSET

#define CLS_X86_FRAMELESS_STACK_SIZE UNWIND_X86_64_FRAMELESS_STACK_SIZE
#define CLS_X86_FRAMELESS_STACK_ADJUST UNWIND_X86_64_FRAMELESS_STACK_ADJUST
#define CLS_X86_FRAMELESS_STACK_REG_COUNT UNWIND_X86_64_FRAMELESS_STACK_REG_COUNT
#define CLS_X86_FRAMELESS_STACK_REG_PERMUTATION UNWIND_X86_64_FRAMELESS_STACK_REG_PERMUTATION

#define CLS_X86_DWARF_SECTION_OFFSET UNWIND_X86_64_DWARF_SECTION_OFFSET

#define CLS_X86_REG_RBP UNWIND_X86_64_REG_RBP

#else

#define CLS_X86_MODE_MASK UNWIND_X86_MODE_MASK
#define CLS_X86_MODE_BP_FRAME UNWIND_X86_MODE_EBP_FRAME
#define CLS_X86_MODE_STACK_IMMD UNWIND_X86_MODE_STACK_IMMD
#define CLS_X86_MODE_STACK_IND UNWIND_X86_MODE_STACK_IND
#define CLS_X86_MODE_DWARF UNWIND_X86_MODE_DWARF

#define CLS_X86_BP_FRAME_REGISTERS UNWIND_X86_RBP_FRAME_REGISTERS
#define CLS_X86_BP_FRAME_OFFSET UNWIND_X86_RBP_FRAME_OFFSET

#define CLS_X86_FRAMELESS_STACK_SIZE UNWIND_X86_FRAMELESS_STACK_SIZE
#define CLS_X86_FRAMELESS_STACK_ADJUST UNWIND_X86_FRAMELESS_STACK_ADJUST
#define CLS_X86_FRAMELESS_STACK_REG_COUNT UNWIND_X86_FRAMELESS_STACK_REG_COUNT
#define CLS_X86_FRAMELESS_STACK_REG_PERMUTATION UNWIND_X86_FRAMELESS_STACK_REG_PERMUTATION

#define CLS_X86_DWARF_SECTION_OFFSET UNWIND_X86_DWARF_SECTION_OFFSET

#define CLS_X86_REG_RBP UNWIND_X86_REG_EBP

#endif

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSCompactUnwindComputeStackSize(const compact_unwind_encoding_t encoding,
                                         const uintptr_t functionStart,
                                         const bool indirect,
                                         uint32_t* const stackSize);
bool FIRCLSCompactUnwindDecompressPermutation(const compact_unwind_encoding_t encoding,
                                              uintptr_t permutatedRegisters[const static 6]);
bool FIRCLSCompactUnwindRestoreRegisters(compact_unwind_encoding_t encoding,
                                         FIRCLSThreadContext* registers,
                                         uint32_t stackSize,
                                         const uintptr_t savedRegisters[const static 6],
                                         uintptr_t* address);
#endif

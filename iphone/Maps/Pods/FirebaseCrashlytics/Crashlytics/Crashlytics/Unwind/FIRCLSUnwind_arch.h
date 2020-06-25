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

#include "FIRCLSFeatures.h"
#include "FIRCLSThreadState.h"
#if CLS_COMPACT_UNWINDING_SUPPORTED
#include "FIRCLSCompactUnwind.h"
#endif

bool FIRCLSUnwindWithFramePointer(FIRCLSThreadContext *registers, bool allowScanning);
uintptr_t FIRCLSUnwindStackPointerFromFramePointer(uintptr_t framePtr);

#if CLS_DWARF_UNWINDING_SUPPORTED
uintptr_t FIRCLSCompactUnwindDwarfOffset(compact_unwind_encoding_t encoding);
bool FIRCLSDwarfUnwindSetRegisterValue(FIRCLSThreadContext *registers,
                                       uint64_t num,
                                       uintptr_t value);
uintptr_t FIRCLSDwarfUnwindGetRegisterValue(const FIRCLSThreadContext *registers, uint64_t num);
#endif

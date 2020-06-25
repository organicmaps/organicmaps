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

#include "FIRCLSDefines.h"

#define CLS_MEMORY_PROTECTION_ENABLED 1
#define CLS_COMPACT_UNWINDED_ENABLED 1
#define CLS_DWARF_UNWINDING_ENABLED 1

#define CLS_USE_SIGALTSTACK (!TARGET_OS_WATCH && !TARGET_OS_TV)
#define CLS_CAN_SUSPEND_THREADS !TARGET_OS_WATCH
#define CLS_MACH_EXCEPTION_SUPPORTED (!TARGET_OS_WATCH && !TARGET_OS_TV)

#define CLS_COMPACT_UNWINDING_SUPPORTED \
  ((CLS_CPU_I386 || CLS_CPU_X86_64 || CLS_CPU_ARM64) && CLS_COMPACT_UNWINDED_ENABLED)

#define CLS_DWARF_UNWINDING_SUPPORTED \
  (CLS_COMPACT_UNWINDING_SUPPORTED && CLS_DWARF_UNWINDING_ENABLED)

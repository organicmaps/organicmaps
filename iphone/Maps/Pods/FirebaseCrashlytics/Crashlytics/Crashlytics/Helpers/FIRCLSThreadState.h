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
#include <sys/ucontext.h>

#if CLS_CPU_ARM
#define FIRCLSThreadStateCount ARM_THREAD_STATE_COUNT
#define FIRCLSThreadState ARM_THREAD_STATE
#elif CLS_CPU_ARM64
#define FIRCLSThreadStateCount ARM_THREAD_STATE64_COUNT
#define FIRCLSThreadState ARM_THREAD_STATE64
#elif CLS_CPU_I386
#define FIRCLSThreadStateCount x86_THREAD_STATE32_COUNT
#define FIRCLSThreadState x86_THREAD_STATE32
#elif CLS_CPU_X86_64
#define FIRCLSThreadStateCount x86_THREAD_STATE64_COUNT
#define FIRCLSThreadState x86_THREAD_STATE64
#endif

// _STRUCT_MCONTEXT was fixed to point to the right thing on ARM in the iOS 7.1 SDK
typedef _STRUCT_MCONTEXT FIRCLSThreadContext;

// I'm not entirely sure what happened when, but this appears to have disappeared from
// the SDKs...
#if !defined(_STRUCT_UCONTEXT64)
typedef _STRUCT_UCONTEXT _STRUCT_UCONTEXT64;
#endif

#pragma mark Register Access

uintptr_t FIRCLSThreadContextGetPC(FIRCLSThreadContext* registers);
uintptr_t FIRCLSThreadContextGetStackPointer(const FIRCLSThreadContext* registers);
uintptr_t FIRCLSThreadContextGetFramePointer(const FIRCLSThreadContext* registers);

bool FIRCLSThreadContextSetPC(FIRCLSThreadContext* registers, uintptr_t value);
bool FIRCLSThreadContextSetStackPointer(FIRCLSThreadContext* registers, uintptr_t value);
bool FIRCLSThreadContextSetFramePointer(FIRCLSThreadContext* registers, uintptr_t value);

// The link register only exists on ARM platforms.
#if CLS_CPU_ARM || CLS_CPU_ARM64
uintptr_t FIRCLSThreadContextGetLinkRegister(const FIRCLSThreadContext* registers);
bool FIRCLSThreadContextSetLinkRegister(FIRCLSThreadContext* registers, uintptr_t value);
#endif

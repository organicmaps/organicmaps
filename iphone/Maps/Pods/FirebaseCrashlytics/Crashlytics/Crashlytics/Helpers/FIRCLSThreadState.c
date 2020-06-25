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

#include "FIRCLSThreadState.h"
#include "FIRCLSDefines.h"
#include "FIRCLSUtility.h"

#if defined(__arm__) || defined(__arm64__)
#include <mach/arm/thread_status.h>
#include <ptrauth.h>
#endif

#if CLS_CPU_X86_64
#define GET_IP_REGISTER(r) (r->__ss.__rip)
#define GET_FP_REGISTER(r) (r->__ss.__rbp)
#define GET_SP_REGISTER(r) (r->__ss.__rsp)
#define GET_LR_REGISTER(r) 0
#define SET_IP_REGISTER(r, v) (r->__ss.__rip = v)
#define SET_FP_REGISTER(r, v) (r->__ss.__rbp = v)
#define SET_SP_REGISTER(r, v) (r->__ss.__rsp = v)
#define SET_LR_REGISTER(r, v)
#elif CLS_CPU_I386
#define GET_IP_REGISTER(r) (r->__ss.__eip)
#define GET_FP_REGISTER(r) (r->__ss.__ebp)
#define GET_SP_REGISTER(r) (r->__ss.__esp)
#define GET_LR_REGISTER(r) 0
#define SET_IP_REGISTER(r, v) (r->__ss.__eip = v)
#define SET_FP_REGISTER(r, v) (r->__ss.__ebp = v)
#define SET_SP_REGISTER(r, v) (r->__ss.__esp = v)
#define SET_LR_REGISTER(r, v)
#elif CLS_CPU_ARM64
// The arm_thread_state64_get_* macros translate down to the AUTIA and AUTIB instructions which
// authenticate the address, but don't clear the upper bits. From the docs:
//      "If the authentication passes, the upper bits of the address are restored to enable
//      subsequent use of the address. the authentication fails, the upper bits are corrupted and
//      any subsequent use of the address results in a Translation fault."
// Since we only want the address (with the metadata in the upper bits masked out), we used the
// ptrauth_strip macro to clear the upper bits.
//
// We found later that ptrauth_strip doesn't seem to do anything. In many cases, the upper bits were
// already stripped, so for most non-system-library code, Crashlytics would still symbolicate. But
// for system libraries, the upper bits were being left in even when we called ptrauth_strip.
// Instead, we're bit masking and only allowing the latter 36 bits.
#define CLS_PTRAUTH_STRIP(pointer) ((uintptr_t)pointer & 0x0000000FFFFFFFFF)
#define GET_IP_REGISTER(r) (CLS_PTRAUTH_STRIP(arm_thread_state64_get_pc(r->__ss)))
#define GET_FP_REGISTER(r) (CLS_PTRAUTH_STRIP(arm_thread_state64_get_fp(r->__ss)))
#define GET_SP_REGISTER(r) (CLS_PTRAUTH_STRIP(arm_thread_state64_get_sp(r->__ss)))
#define GET_LR_REGISTER(r) (CLS_PTRAUTH_STRIP(arm_thread_state64_get_lr(r->__ss)))
#define SET_IP_REGISTER(r, v) arm_thread_state64_set_pc_fptr(r->__ss, (void*)v)
#define SET_FP_REGISTER(r, v) arm_thread_state64_set_fp(r->__ss, v)
#define SET_SP_REGISTER(r, v) arm_thread_state64_set_sp(r->__ss, v)
#define SET_LR_REGISTER(r, v) arm_thread_state64_set_lr_fptr(r->__ss, (void*)v)
#elif CLS_CPU_ARM
#define GET_IP_REGISTER(r) (r->__ss.__pc)
#define GET_FP_REGISTER(r) (r->__ss.__r[7])
#define GET_SP_REGISTER(r) (r->__ss.__sp)
#define GET_LR_REGISTER(r) (r->__ss.__lr)
#define SET_IP_REGISTER(r, v) (r->__ss.__pc = v)
#define SET_FP_REGISTER(r, v) (r->__ss.__r[7] = v)
#define SET_SP_REGISTER(r, v) (r->__ss.__sp = v)
#define SET_LR_REGISTER(r, v) (r->__ss.__lr = v)
#else
#error "Architecture Unsupported"
#endif

uintptr_t FIRCLSThreadContextGetPC(FIRCLSThreadContext* registers) {
  if (!registers) {
    return 0;
  }

  return GET_IP_REGISTER(registers);
}

uintptr_t FIRCLSThreadContextGetStackPointer(const FIRCLSThreadContext* registers) {
  if (!registers) {
    return 0;
  }

  return GET_SP_REGISTER(registers);
}

bool FIRCLSThreadContextSetStackPointer(FIRCLSThreadContext* registers, uintptr_t value) {
  if (!FIRCLSIsValidPointer(registers)) {
    return false;
  }

  SET_SP_REGISTER(registers, value);

  return true;
}

uintptr_t FIRCLSThreadContextGetLinkRegister(const FIRCLSThreadContext* registers) {
  if (!FIRCLSIsValidPointer(registers)) {
    return 0;
  }

  return GET_LR_REGISTER(registers);
}

bool FIRCLSThreadContextSetLinkRegister(FIRCLSThreadContext* registers, uintptr_t value) {
  if (!FIRCLSIsValidPointer(registers)) {
    return false;
  }

  SET_LR_REGISTER(registers, value);

  return true;
}

bool FIRCLSThreadContextSetPC(FIRCLSThreadContext* registers, uintptr_t value) {
  if (!registers) {
    return false;
  }

  SET_IP_REGISTER(registers, value);

  return true;
}

uintptr_t FIRCLSThreadContextGetFramePointer(const FIRCLSThreadContext* registers) {
  if (!FIRCLSIsValidPointer(registers)) {
    return 0;
  }

  return GET_FP_REGISTER(registers);
}

bool FIRCLSThreadContextSetFramePointer(FIRCLSThreadContext* registers, uintptr_t value) {
  if (!FIRCLSIsValidPointer(registers)) {
    return false;
  }

  SET_FP_REGISTER(registers, value);

  return true;
}

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

#include "FIRCLSCompactUnwind.h"
#include "FIRCLSCompactUnwind_Private.h"
#include "FIRCLSDefines.h"
#include "FIRCLSDwarfUnwind.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSUnwind.h"
#include "FIRCLSUnwind_arch.h"
#include "FIRCLSUtility.h"

#if CLS_CPU_ARM || CLS_CPU_ARM64

static bool FIRCLSUnwindWithLRRegister(FIRCLSThreadContext* registers) {
  if (!FIRCLSIsValidPointer(registers)) {
    return false;
  }

  // Return address is in LR, SP is pointing to the next frame.
  uintptr_t value = FIRCLSThreadContextGetLinkRegister(registers);

  if (!FIRCLSIsValidPointer(value)) {
    FIRCLSSDKLog("Error: LR value is invalid\n");
    return false;
  }

  return FIRCLSThreadContextSetPC(registers, value);
}

bool FIRCLSUnwindWithFramePointer(FIRCLSThreadContext* registers, bool allowScanning) {
  if (allowScanning) {
    // The LR register does have the return address here, but there are situations where
    // this can produce false matches. Better backend rules can fix this up in many cases.
    if (FIRCLSUnwindWithLRRegister(registers)) {
      return true;
    } else {
      // In this case, we're unable to use the LR. We don't want to just stop unwinding, so
      // proceed with the normal, non-scanning path
      FIRCLSSDKLog("Unable to use LR, skipping\n");
    }
  }

  // read the values from the stack
  const uintptr_t framePointer = FIRCLSThreadContextGetFramePointer(registers);
  uintptr_t stack[2];

  if (!FIRCLSReadMemory((vm_address_t)framePointer, stack, sizeof(stack))) {
    // unable to read the first stack frame
    FIRCLSSDKLog("Error: failed to read memory at address %p\n", (void*)framePointer);
    return false;
  }

  if (!FIRCLSThreadContextSetPC(registers, stack[1])) {
    return false;
  }

  if (!FIRCLSThreadContextSetFramePointer(registers, stack[0])) {
    return false;
  }

  if (!FIRCLSThreadContextSetStackPointer(registers,
                                          FIRCLSUnwindStackPointerFromFramePointer(framePointer))) {
    return false;
  }

  return true;
}

uintptr_t FIRCLSUnwindStackPointerFromFramePointer(uintptr_t framePtr) {
  // the stack pointer is the frame pointer plus the two saved pointers for the frame
  return framePtr + 2 * sizeof(void*);
}

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSCompactUnwindComputeRegisters(FIRCLSCompactUnwindContext* context,
                                         FIRCLSCompactUnwindResult* result,
                                         FIRCLSThreadContext* registers) {
  if (!context || !result || !registers) {
    return false;
  }

  // Note that compact_uwnind_encoding.h has a few bugs in it prior to iOS 8.0.
  // Only refer to the >= 8.0 header.
  switch (result->encoding & UNWIND_ARM64_MODE_MASK) {
    case UNWIND_ARM64_MODE_FRAMELESS:
      // Interestingly, we also know the size of the stack frame, by
      // using UNWIND_ARM64_FRAMELESS_STACK_SIZE_MASK. Is that useful?
      return FIRCLSUnwindWithLRRegister(registers);
      break;
    case UNWIND_ARM64_MODE_DWARF:
      return FIRCLSCompactUnwindDwarfFrame(
          context, result->encoding & UNWIND_ARM64_DWARF_SECTION_OFFSET, registers);
      break;
    case UNWIND_ARM64_MODE_FRAME:
      return FIRCLSUnwindWithFramePointer(registers, false);
    default:
      FIRCLSSDKLog("Invalid encoding 0x%x\n", result->encoding);
      break;
  }

  return false;
}
#endif

#if CLS_DWARF_UNWINDING_SUPPORTED
uintptr_t FIRCLSDwarfUnwindGetRegisterValue(const FIRCLSThreadContext* registers, uint64_t num) {
  switch (num) {
    case CLS_DWARF_ARM64_X0:
      return registers->__ss.__x[0];
    case CLS_DWARF_ARM64_X1:
      return registers->__ss.__x[1];
    case CLS_DWARF_ARM64_X2:
      return registers->__ss.__x[2];
    case CLS_DWARF_ARM64_X3:
      return registers->__ss.__x[3];
    case CLS_DWARF_ARM64_X4:
      return registers->__ss.__x[4];
    case CLS_DWARF_ARM64_X5:
      return registers->__ss.__x[5];
    case CLS_DWARF_ARM64_X6:
      return registers->__ss.__x[6];
    case CLS_DWARF_ARM64_X7:
      return registers->__ss.__x[7];
    case CLS_DWARF_ARM64_X8:
      return registers->__ss.__x[8];
    case CLS_DWARF_ARM64_X9:
      return registers->__ss.__x[9];
    case CLS_DWARF_ARM64_X10:
      return registers->__ss.__x[10];
    case CLS_DWARF_ARM64_X11:
      return registers->__ss.__x[11];
    case CLS_DWARF_ARM64_X12:
      return registers->__ss.__x[12];
    case CLS_DWARF_ARM64_X13:
      return registers->__ss.__x[13];
    case CLS_DWARF_ARM64_X14:
      return registers->__ss.__x[14];
    case CLS_DWARF_ARM64_X15:
      return registers->__ss.__x[15];
    case CLS_DWARF_ARM64_X16:
      return registers->__ss.__x[16];
    case CLS_DWARF_ARM64_X17:
      return registers->__ss.__x[17];
    case CLS_DWARF_ARM64_X18:
      return registers->__ss.__x[18];
    case CLS_DWARF_ARM64_X19:
      return registers->__ss.__x[19];
    case CLS_DWARF_ARM64_X20:
      return registers->__ss.__x[20];
    case CLS_DWARF_ARM64_X21:
      return registers->__ss.__x[21];
    case CLS_DWARF_ARM64_X22:
      return registers->__ss.__x[22];
    case CLS_DWARF_ARM64_X23:
      return registers->__ss.__x[23];
    case CLS_DWARF_ARM64_X24:
      return registers->__ss.__x[24];
    case CLS_DWARF_ARM64_X25:
      return registers->__ss.__x[25];
    case CLS_DWARF_ARM64_X26:
      return registers->__ss.__x[26];
    case CLS_DWARF_ARM64_X27:
      return registers->__ss.__x[27];
    case CLS_DWARF_ARM64_X28:
      return registers->__ss.__x[28];
    case CLS_DWARF_ARM64_FP:
      return FIRCLSThreadContextGetFramePointer(registers);
    case CLS_DWARF_ARM64_LR:
      return FIRCLSThreadContextGetLinkRegister(registers);
    case CLS_DWARF_ARM64_SP:
      return FIRCLSThreadContextGetStackPointer(registers);
    default:
      break;
  }

  FIRCLSSDKLog("Error: Unrecognized get register number %llu\n", num);

  return 0;
}

bool FIRCLSDwarfUnwindSetRegisterValue(FIRCLSThreadContext* registers,
                                       uint64_t num,
                                       uintptr_t value) {
  switch (num) {
    case CLS_DWARF_ARM64_X0:
      registers->__ss.__x[0] = value;
      return true;
    case CLS_DWARF_ARM64_X1:
      registers->__ss.__x[1] = value;
      return true;
    case CLS_DWARF_ARM64_X2:
      registers->__ss.__x[2] = value;
      return true;
    case CLS_DWARF_ARM64_X3:
      registers->__ss.__x[3] = value;
      return true;
    case CLS_DWARF_ARM64_X4:
      registers->__ss.__x[4] = value;
      return true;
    case CLS_DWARF_ARM64_X5:
      registers->__ss.__x[5] = value;
      return true;
    case CLS_DWARF_ARM64_X6:
      registers->__ss.__x[6] = value;
      return true;
    case CLS_DWARF_ARM64_X7:
      registers->__ss.__x[7] = value;
      return true;
    case CLS_DWARF_ARM64_X8:
      registers->__ss.__x[8] = value;
      return true;
    case CLS_DWARF_ARM64_X9:
      registers->__ss.__x[9] = value;
      return true;
    case CLS_DWARF_ARM64_X10:
      registers->__ss.__x[10] = value;
      return true;
    case CLS_DWARF_ARM64_X11:
      registers->__ss.__x[11] = value;
      return true;
    case CLS_DWARF_ARM64_X12:
      registers->__ss.__x[12] = value;
      return true;
    case CLS_DWARF_ARM64_X13:
      registers->__ss.__x[13] = value;
      return true;
    case CLS_DWARF_ARM64_X14:
      registers->__ss.__x[14] = value;
      return true;
    case CLS_DWARF_ARM64_X15:
      registers->__ss.__x[15] = value;
      return true;
    case CLS_DWARF_ARM64_X16:
      registers->__ss.__x[16] = value;
      return true;
    case CLS_DWARF_ARM64_X17:
      registers->__ss.__x[17] = value;
      return true;
    case CLS_DWARF_ARM64_X18:
      registers->__ss.__x[18] = value;
      return true;
    case CLS_DWARF_ARM64_X19:
      registers->__ss.__x[19] = value;
      return true;
    case CLS_DWARF_ARM64_X20:
      registers->__ss.__x[20] = value;
      return true;
    case CLS_DWARF_ARM64_X21:
      registers->__ss.__x[21] = value;
      return true;
    case CLS_DWARF_ARM64_X22:
      registers->__ss.__x[22] = value;
      return true;
    case CLS_DWARF_ARM64_X23:
      registers->__ss.__x[23] = value;
      return true;
    case CLS_DWARF_ARM64_X24:
      registers->__ss.__x[24] = value;
      return true;
    case CLS_DWARF_ARM64_X25:
      registers->__ss.__x[25] = value;
      return true;
    case CLS_DWARF_ARM64_X26:
      registers->__ss.__x[26] = value;
      return true;
    case CLS_DWARF_ARM64_X27:
      registers->__ss.__x[27] = value;
      return true;
    case CLS_DWARF_ARM64_X28:
      registers->__ss.__x[28] = value;
      return true;
    case CLS_DWARF_ARM64_FP:
      FIRCLSThreadContextSetFramePointer(registers, value);
      return true;
    case CLS_DWARF_ARM64_SP:
      FIRCLSThreadContextSetStackPointer(registers, value);
      return true;
    case CLS_DWARF_ARM64_LR:
      // Here's what's going on. For x86, the "return register" is virtual. The architecture
      // doesn't actually have one, but DWARF does have the concept. So, when the system
      // tries to set the return register, we set the PC. You can see this behavior
      // in the FIRCLSDwarfUnwindSetRegisterValue implemenation for that architecture. In the
      // case of ARM64, the register is real. So, we have to be extra careful to make sure
      // we update the PC here. Otherwise, when a DWARF unwind completes, it won't have
      // changed the PC to the right value.
      FIRCLSThreadContextSetLinkRegister(registers, value);
      FIRCLSThreadContextSetPC(registers, value);
      return true;
    default:
      break;
  }

  FIRCLSSDKLog("Unrecognized set register number %llu\n", num);

  return false;
}
#endif

#else
INJECT_STRIP_SYMBOL(unwind_arm)
#endif

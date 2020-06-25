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

#include "FIRCLSUnwind_x86.h"
#include "FIRCLSCompactUnwind_Private.h"
#include "FIRCLSDefines.h"
#include "FIRCLSDwarfUnwind.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSUnwind.h"
#include "FIRCLSUnwind_arch.h"
#include "FIRCLSUtility.h"

#if CLS_CPU_X86

static bool FIRCLSCompactUnwindBPFrame(compact_unwind_encoding_t encoding,
                                       FIRCLSThreadContext* registers);
static bool FIRCLSCompactUnwindFrameless(compact_unwind_encoding_t encoding,
                                         FIRCLSThreadContext* registers,
                                         uintptr_t functionStart,
                                         bool indirect);

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSCompactUnwindComputeRegisters(FIRCLSCompactUnwindContext* context,
                                         FIRCLSCompactUnwindResult* result,
                                         FIRCLSThreadContext* registers) {
  if (!FIRCLSIsValidPointer(context) || !FIRCLSIsValidPointer(result) ||
      !FIRCLSIsValidPointer(registers)) {
    FIRCLSSDKLogError("invalid inputs\n");
    return false;
  }

  FIRCLSSDKLogDebug("Computing registers for encoding %x\n", result->encoding);

  switch (result->encoding & CLS_X86_MODE_MASK) {
    case CLS_X86_MODE_BP_FRAME:
      return FIRCLSCompactUnwindBPFrame(result->encoding, registers);
    case CLS_X86_MODE_STACK_IMMD:
      return FIRCLSCompactUnwindFrameless(result->encoding, registers, result->functionStart,
                                          false);
    case CLS_X86_MODE_STACK_IND:
      return FIRCLSCompactUnwindFrameless(result->encoding, registers, result->functionStart, true);
    case CLS_X86_MODE_DWARF:
      return FIRCLSCompactUnwindDwarfFrame(context, result->encoding & CLS_X86_DWARF_SECTION_OFFSET,
                                           registers);
    default:
      FIRCLSSDKLogError("Invalid encoding %x\n", result->encoding);
      break;
  }

  return false;
}
#endif

static bool FIRCLSCompactUnwindBPFrame(compact_unwind_encoding_t encoding,
                                       FIRCLSThreadContext* registers) {
  // this is the plain-vanilla frame pointer process

  // uint32_t offset = GET_BITS_WITH_MASK(encoding, UNWIND_X86_EBP_FRAME_OFFSET);
  // uint32_t locations = GET_BITS_WITH_MASK(encoding, UNWIND_X86_64_RBP_FRAME_REGISTERS);

  // TODO: pretty sure we do need to restore registers here, so that if a subsequent frame needs
  // these results, they will be correct

  // Checkout CompactUnwinder.hpp in libunwind for how to do this. Since we don't make use of any of
  // those registers for a stacktrace only, there's nothing we need do with them.

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

bool FIRCLSUnwindWithStackScanning(FIRCLSThreadContext* registers) {
  vm_address_t start = (vm_address_t)FIRCLSThreadContextGetStackPointer(registers);
  vm_address_t end = (vm_address_t)FIRCLSThreadContextGetFramePointer(registers);

  uintptr_t newPC = 0;

  if (!FIRCLSUnwindFirstExecutableAddress(start, end, (vm_address_t*)&newPC)) {
    return false;
  }

  return FIRCLSThreadContextSetPC(registers, newPC);
}

bool FIRCLSUnwindWithFramePointer(FIRCLSThreadContext* registers, bool allowScanning) {
  // Here's an interesting case. We've just processed the first frame, and it did
  // not have any unwind info. If that first function did not allocate
  // a stack frame, we'll "skip" the caller. This might sound unlikely, but it actually
  // happens a lot in practice.

  // Sooo, one thing we can do is try to stack the stack for things that look like return
  // addresses. Normally, this technique will hit many false positives. But, if we do it
  // only for the second frame, and only when we don't have other unwind info available.

  if (allowScanning) {
    FIRCLSSDKLogInfo("Attempting stack scan\n");
    if (FIRCLSUnwindWithStackScanning(registers)) {
      FIRCLSSDKLogInfo("Stack scan successful\n");
      return true;
    }
  }

  // If we ever do anything else with the encoding, we need to be sure
  // to set it up right.
  return FIRCLSCompactUnwindBPFrame(CLS_X86_MODE_BP_FRAME, registers);
}

uintptr_t FIRCLSUnwindStackPointerFromFramePointer(uintptr_t framePtr) {
  // the stack pointer is the frame pointer plus the two saved pointers for the frame
  return framePtr + 2 * sizeof(void*);
}

#if CLS_COMPACT_UNWINDING_SUPPORTED || CLS_DWARF_UNWINDING_SUPPORTED
uintptr_t FIRCLSDwarfUnwindGetRegisterValue(const FIRCLSThreadContext* registers, uint64_t num) {
  switch (num) {
#if CLS_CPU_X86_64
    case CLS_DWARF_X86_64_RAX:
      return registers->__ss.__rax;
    case CLS_DWARF_X86_64_RDX:
      return registers->__ss.__rdx;
    case CLS_DWARF_X86_64_RCX:
      return registers->__ss.__rcx;
    case CLS_DWARF_X86_64_RBX:
      return registers->__ss.__rbx;
    case CLS_DWARF_X86_64_RSI:
      return registers->__ss.__rsi;
    case CLS_DWARF_X86_64_RDI:
      return registers->__ss.__rdi;
    case CLS_DWARF_X86_64_RBP:
      return registers->__ss.__rbp;
    case CLS_DWARF_X86_64_RSP:
      return registers->__ss.__rsp;
    case CLS_DWARF_X86_64_R8:
      return registers->__ss.__r8;
    case CLS_DWARF_X86_64_R9:
      return registers->__ss.__r9;
    case CLS_DWARF_X86_64_R10:
      return registers->__ss.__r10;
    case CLS_DWARF_X86_64_R11:
      return registers->__ss.__r11;
    case CLS_DWARF_X86_64_R12:
      return registers->__ss.__r12;
    case CLS_DWARF_X86_64_R13:
      return registers->__ss.__r13;
    case CLS_DWARF_X86_64_R14:
      return registers->__ss.__r14;
    case CLS_DWARF_X86_64_R15:
      return registers->__ss.__r15;
    case CLS_DWARF_X86_64_RET_ADDR:
      return registers->__ss.__rip;
#elif CLS_CPU_I386
    case CLS_DWARF_X86_EAX:
      return registers->__ss.__eax;
    case CLS_DWARF_X86_ECX:
      return registers->__ss.__ecx;
    case CLS_DWARF_X86_EDX:
      return registers->__ss.__edx;
    case CLS_DWARF_X86_EBX:
      return registers->__ss.__ebx;
    case CLS_DWARF_X86_EBP:
      return registers->__ss.__ebp;
    case CLS_DWARF_X86_ESP:
      return registers->__ss.__esp;
    case CLS_DWARF_X86_ESI:
      return registers->__ss.__esi;
    case CLS_DWARF_X86_EDI:
      return registers->__ss.__edi;
    case CLS_DWARF_X86_RET_ADDR:
      return registers->__ss.__eip;
#endif
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
#if CLS_CPU_X86_64
    case CLS_DWARF_X86_64_RAX:
      registers->__ss.__rax = value;
      return true;
    case CLS_DWARF_X86_64_RDX:
      registers->__ss.__rdx = value;
      return true;
    case CLS_DWARF_X86_64_RCX:
      registers->__ss.__rcx = value;
      return true;
    case CLS_DWARF_X86_64_RBX:
      registers->__ss.__rbx = value;
      return true;
    case CLS_DWARF_X86_64_RSI:
      registers->__ss.__rsi = value;
      return true;
    case CLS_DWARF_X86_64_RDI:
      registers->__ss.__rdi = value;
      return true;
    case CLS_DWARF_X86_64_RBP:
      registers->__ss.__rbp = value;
      return true;
    case CLS_DWARF_X86_64_RSP:
      registers->__ss.__rsp = value;
      return true;
    case CLS_DWARF_X86_64_R8:
      registers->__ss.__r8 = value;
      return true;
    case CLS_DWARF_X86_64_R9:
      registers->__ss.__r9 = value;
      return true;
    case CLS_DWARF_X86_64_R10:
      registers->__ss.__r10 = value;
      return true;
    case CLS_DWARF_X86_64_R11:
      registers->__ss.__r11 = value;
      return true;
    case CLS_DWARF_X86_64_R12:
      registers->__ss.__r12 = value;
      return true;
    case CLS_DWARF_X86_64_R13:
      registers->__ss.__r13 = value;
      return true;
    case CLS_DWARF_X86_64_R14:
      registers->__ss.__r14 = value;
      return true;
    case CLS_DWARF_X86_64_R15:
      registers->__ss.__r15 = value;
      return true;
    case CLS_DWARF_X86_64_RET_ADDR:
      registers->__ss.__rip = value;
      return true;
#elif CLS_CPU_I386
    case CLS_DWARF_X86_EAX:
      registers->__ss.__eax = value;
      return true;
    case CLS_DWARF_X86_ECX:
      registers->__ss.__ecx = value;
      return true;
    case CLS_DWARF_X86_EDX:
      registers->__ss.__edx = value;
      return true;
    case CLS_DWARF_X86_EBX:
      registers->__ss.__ebx = value;
      return true;
    case CLS_DWARF_X86_EBP:
      registers->__ss.__ebp = value;
      return true;
    case CLS_DWARF_X86_ESP:
      registers->__ss.__esp = value;
      return true;
    case CLS_DWARF_X86_ESI:
      registers->__ss.__esi = value;
      return true;
    case CLS_DWARF_X86_EDI:
      registers->__ss.__edi = value;
      return true;
    case CLS_DWARF_X86_RET_ADDR:
      registers->__ss.__eip = value;
      return true;
#endif
    default:
      break;
  }

  FIRCLSSDKLog("Unrecognized set register number %llu\n", num);

  return false;
}
#endif

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSCompactUnwindComputeStackSize(const compact_unwind_encoding_t encoding,
                                         const uintptr_t functionStart,
                                         const bool indirect,
                                         uint32_t* const stackSize) {
  if (!FIRCLSIsValidPointer(stackSize)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return false;
  }

  const uint32_t stackSizeEncoded = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_SIZE);

  if (!indirect) {
    *stackSize = stackSizeEncoded * sizeof(void*);
    return true;
  }

  const vm_address_t sublAddress = functionStart + stackSizeEncoded;
  uint32_t sublValue = 0;

  if (!FIRCLSReadMemory(sublAddress, &sublValue, sizeof(uint32_t))) {
    FIRCLSSDKLog("Error: unable to read subl value\n");
    return false;
  }

  const uint32_t stackAdjust = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_ADJUST);

  *stackSize = sublValue + stackAdjust * sizeof(void*);

  return true;
}

bool FIRCLSCompactUnwindDecompressPermutation(const compact_unwind_encoding_t encoding,
                                              uintptr_t permutatedRegisters[const static 6]) {
  const uint32_t regCount = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_REG_COUNT);
  uint32_t permutation = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_REG_PERMUTATION);

  switch (regCount) {
    case 6:
      permutatedRegisters[0] = permutation / 120;
      permutation -= (permutatedRegisters[0] * 120);
      permutatedRegisters[1] = permutation / 24;
      permutation -= (permutatedRegisters[1] * 24);
      permutatedRegisters[2] = permutation / 6;
      permutation -= (permutatedRegisters[2] * 6);
      permutatedRegisters[3] = permutation / 2;
      permutation -= (permutatedRegisters[3] * 2);
      permutatedRegisters[4] = permutation;
      permutatedRegisters[5] = 0;
      break;
    case 5:
      permutatedRegisters[0] = permutation / 120;
      permutation -= (permutatedRegisters[0] * 120);
      permutatedRegisters[1] = permutation / 24;
      permutation -= (permutatedRegisters[1] * 24);
      permutatedRegisters[2] = permutation / 6;
      permutation -= (permutatedRegisters[2] * 6);
      permutatedRegisters[3] = permutation / 2;
      permutation -= (permutatedRegisters[3] * 2);
      permutatedRegisters[4] = permutation;
      break;
    case 4:
      permutatedRegisters[0] = permutation / 60;
      permutation -= (permutatedRegisters[0] * 60);
      permutatedRegisters[1] = permutation / 12;
      permutation -= (permutatedRegisters[1] * 12);
      permutatedRegisters[2] = permutation / 3;
      permutation -= (permutatedRegisters[2] * 3);
      permutatedRegisters[3] = permutation;
      break;
    case 3:
      permutatedRegisters[0] = permutation / 20;
      permutation -= (permutatedRegisters[0] * 20);
      permutatedRegisters[1] = permutation / 4;
      permutation -= (permutatedRegisters[1] * 4);
      permutatedRegisters[2] = permutation;
      break;
    case 2:
      permutatedRegisters[0] = permutation / 5;
      permutation -= (permutatedRegisters[0] * 5);
      permutatedRegisters[1] = permutation;
      break;
    case 1:
      permutatedRegisters[0] = permutation;
      break;
    case 0:
      break;
    default:
      FIRCLSSDKLog("Error: unhandled number of register permutations for encoding %x\n", encoding);
      return false;
  }

  return true;
}

bool FIRCLSCompactUnwindRemapRegisters(const compact_unwind_encoding_t encoding,
                                       uintptr_t permutatedRegisters[const static 6],
                                       uintptr_t savedRegisters[const static 6]) {
  const uint32_t regCount = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_REG_COUNT);

  if (regCount > 6) {
    FIRCLSSDKLog("Error: invalid register number count %d\n", regCount);
    return false;
  }

  // Re-number the registers

  // You are probably wondering, what the hell is this algorithm even doing? It is
  // taken from libunwind's implemenation that does the same thing.
  bool used[7] = {false, false, false, false, false, false, false};
  for (uint32_t i = 0; i < regCount; ++i) {
    int renum = 0;
    for (int u = 1; u < 7; ++u) {
      if (!used[u]) {
        if (renum == permutatedRegisters[i]) {
          savedRegisters[i] = u;
          used[u] = true;
          break;
        }
        ++renum;
      }
    }
  }

  return true;
}

bool FIRCLSCompactUnwindRestoreRegisters(compact_unwind_encoding_t encoding,
                                         FIRCLSThreadContext* registers,
                                         uint32_t stackSize,
                                         const uintptr_t savedRegisters[const static 6],
                                         uintptr_t* address) {
  if (!FIRCLSIsValidPointer(registers) || !FIRCLSIsValidPointer(address)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return false;
  }

  const uint32_t regCount = GET_BITS_WITH_MASK(encoding, CLS_X86_FRAMELESS_STACK_REG_COUNT);

  // compute initial address of saved registers
  *address = FIRCLSThreadContextGetStackPointer(registers) + stackSize - sizeof(void*) -
             sizeof(void*) * regCount;
  uintptr_t value = 0;

  for (uint32_t i = 0; i < regCount; ++i) {
    value = 0;

    switch (savedRegisters[i]) {
      case CLS_X86_REG_RBP:
        if (!FIRCLSReadMemory((vm_address_t)*address, (void*)&value, sizeof(uintptr_t))) {
          FIRCLSSDKLog("Error: unable to read memory to set register\n");
          return false;
        }

        if (!FIRCLSThreadContextSetFramePointer(registers, value)) {
          FIRCLSSDKLog("Error: unable to set FP\n");
          return false;
        }
        break;
      default:
        // here, we are restoring a register we don't need for unwinding
        FIRCLSSDKLog("Error: skipping a restore of register %d at %p\n", (int)savedRegisters[i],
                     (void*)*address);
        break;
    }

    *address += sizeof(void*);
  }

  return true;
}

static bool FIRCLSCompactUnwindFrameless(compact_unwind_encoding_t encoding,
                                         FIRCLSThreadContext* registers,
                                         uintptr_t functionStart,
                                         bool indirect) {
  FIRCLSSDKLog("Frameless unwind encountered with encoding %x\n", encoding);

  uint32_t stackSize = 0;
  if (!FIRCLSCompactUnwindComputeStackSize(encoding, functionStart, indirect, &stackSize)) {
    FIRCLSSDKLog("Error: unable to compute stack size for encoding %x\n", encoding);
    return false;
  }

  uintptr_t permutatedRegisters[6];

  memset(permutatedRegisters, 0, sizeof(permutatedRegisters));
  if (!FIRCLSCompactUnwindDecompressPermutation(encoding, permutatedRegisters)) {
    FIRCLSSDKLog("Error: unable to decompress registers %x\n", encoding);
    return false;
  }

  uintptr_t savedRegisters[6];

  memset(savedRegisters, 0, sizeof(savedRegisters));
  if (!FIRCLSCompactUnwindRemapRegisters(encoding, permutatedRegisters, savedRegisters)) {
    FIRCLSSDKLog("Error: unable to remap registers %x\n", encoding);
    return false;
  }

  uintptr_t address = 0;

  if (!FIRCLSCompactUnwindRestoreRegisters(encoding, registers, stackSize, savedRegisters,
                                           &address)) {
    FIRCLSSDKLog("Error: unable to restore registers\n");
    return false;
  }

  FIRCLSSDKLog("SP is %p and we are reading %p\n",
               (void*)FIRCLSThreadContextGetStackPointer(registers), (void*)address);
  // read the value from the stack, now that we know the address to read
  uintptr_t value = 0;
  if (!FIRCLSReadMemory((vm_address_t)address, (void*)&value, sizeof(uintptr_t))) {
    FIRCLSSDKLog("Error: unable to read memory to set register\n");
    return false;
  }

  FIRCLSSDKLog("Read PC to be %p\n", (void*)value);
  if (!FIRCLSIsValidPointer(value)) {
    FIRCLSSDKLog("Error: computed PC is invalid\n");
    return false;
  }

  return FIRCLSThreadContextSetPC(registers, value) &&
         FIRCLSThreadContextSetStackPointer(registers, address + sizeof(void*));
}
#endif

#else
INJECT_STRIP_SYMBOL(unwind_x86)
#endif

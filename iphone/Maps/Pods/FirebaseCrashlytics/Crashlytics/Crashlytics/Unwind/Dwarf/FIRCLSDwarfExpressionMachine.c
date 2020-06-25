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

#include "FIRCLSDwarfExpressionMachine.h"
#include "FIRCLSDataParsing.h"
#include "FIRCLSDefines.h"
#include "FIRCLSDwarfUnwindRegisters.h"
#include "FIRCLSUnwind_arch.h"
#include "FIRCLSUtility.h"
#include "dwarf.h"

#if CLS_DWARF_UNWINDING_SUPPORTED

static bool FIRCLSDwarfExpressionMachineExecute_bregN(FIRCLSDwarfExpressionMachine *machine,
                                                      uint8_t opcode);
static bool FIRCLSDwarfExpressionMachineExecute_deref(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_plus_uconst(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_and(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_plus(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_dup(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_swap(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_deref_size(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_ne(FIRCLSDwarfExpressionMachine *machine);
static bool FIRCLSDwarfExpressionMachineExecute_litN(FIRCLSDwarfExpressionMachine *machine,
                                                     uint8_t opcode);

#pragma mark -
#pragma mark Stack Implementation
void FIRCLSDwarfExpressionStackInit(FIRCLSDwarfExpressionStack *stack) {
  if (!FIRCLSIsValidPointer(stack)) {
    return;
  }

  memset(stack, 0, sizeof(FIRCLSDwarfExpressionStack));

  stack->pointer = stack->buffer;
}

bool FIRCLSDwarfExpressionStackIsValid(FIRCLSDwarfExpressionStack *stack) {
  if (!FIRCLSIsValidPointer(stack)) {
    return false;
  }

  // check for valid stack pointer
  if (stack->pointer < stack->buffer) {
    return false;
  }

  if (stack->pointer > stack->buffer + CLS_DWARF_EXPRESSION_STACK_SIZE) {
    return false;
  }

  return true;
}

bool FIRCLSDwarfExpressionStackPush(FIRCLSDwarfExpressionStack *stack, intptr_t value) {
  if (!FIRCLSDwarfExpressionStackIsValid(stack)) {
    return false;
  }

  if (stack->pointer == stack->buffer + CLS_DWARF_EXPRESSION_STACK_SIZE) {
    // overflow
    stack->pointer = NULL;
    return false;
  }

  *(stack->pointer) = value;
  stack->pointer += 1;

  return true;
}

intptr_t FIRCLSDwarfExpressionStackPeek(FIRCLSDwarfExpressionStack *stack) {
  if (!FIRCLSDwarfExpressionStackIsValid(stack)) {
    return 0;
  }

  if (stack->pointer == stack->buffer) {
    // underflow
    stack->pointer = NULL;
    return 0;
  }

  return *(stack->pointer - 1);
}

intptr_t FIRCLSDwarfExpressionStackPop(FIRCLSDwarfExpressionStack *stack) {
  if (!FIRCLSDwarfExpressionStackIsValid(stack)) {
    return 0;
  }

  if (stack->pointer == stack->buffer) {
    // underflow
    stack->pointer = NULL;
    return 0;
  }

  stack->pointer -= 1;

  return *(stack->pointer);
}

#pragma mark -
#pragma mark Machine API
bool FIRCLSDwarfExpressionMachineInit(FIRCLSDwarfExpressionMachine *machine,
                                      const void *cursor,
                                      const FIRCLSThreadContext *registers,
                                      intptr_t stackValue) {
  if (!FIRCLSIsValidPointer(machine)) {
    return false;
  }

  memset(machine, 0, sizeof(FIRCLSDwarfExpressionMachine));

  if (!FIRCLSIsValidPointer(cursor)) {
    return false;
  }

  machine->dataCursor = cursor;
  machine->registers = registers;

  FIRCLSDwarfExpressionStackInit(&machine->stack);

  return FIRCLSDwarfExpressionStackPush(&machine->stack, stackValue);
}

bool FIRCLSDwarfExpressionMachinePrepareForExecution(FIRCLSDwarfExpressionMachine *machine) {
  if (!FIRCLSIsValidPointer(machine)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return false;
  }

  uint64_t expressionLength = FIRCLSParseULEB128AndAdvance(&machine->dataCursor);

  if (expressionLength == 0) {
    FIRCLSSDKLog("Error: DWARF expression length is zero\n");
    return false;
  }

  machine->endAddress = machine->dataCursor + expressionLength;

  return true;
}

bool FIRCLSDwarfExpressionMachineIsFinished(FIRCLSDwarfExpressionMachine *machine) {
  if (!FIRCLSIsValidPointer(machine)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return true;
  }

  if (!FIRCLSIsValidPointer(machine->endAddress) || !FIRCLSIsValidPointer(machine->dataCursor)) {
    FIRCLSSDKLog("Error: DWARF machine pointers invalid\n");
    return true;
  }

  if (!FIRCLSDwarfExpressionStackIsValid(&machine->stack)) {
    FIRCLSSDKLog("Error: DWARF machine stack invalid\n");
    return true;
  }

  return machine->dataCursor >= machine->endAddress;
}

bool FIRCLSDwarfExpressionMachineGetResult(FIRCLSDwarfExpressionMachine *machine,
                                           intptr_t *result) {
  if (!FIRCLSIsValidPointer(machine) || !FIRCLSIsValidPointer(result)) {
    return false;
  }

  if (machine->dataCursor != machine->endAddress) {
    FIRCLSSDKLog("Error: DWARF expression hasn't completed execution\n");
    return false;
  }

  *result = FIRCLSDwarfExpressionStackPeek(&machine->stack);

  return FIRCLSDwarfExpressionStackIsValid(&machine->stack);
}

bool FIRCLSDwarfExpressionMachineExecuteNextOpcode(FIRCLSDwarfExpressionMachine *machine) {
  if (!FIRCLSIsValidPointer(machine)) {
    return false;
  }

  const uint8_t opcode = FIRCLSParseUint8AndAdvance(&machine->dataCursor);

  bool success = false;

  switch (opcode) {
    case DW_OP_deref:
      success = FIRCLSDwarfExpressionMachineExecute_deref(machine);
      break;
    case DW_OP_dup:
      success = FIRCLSDwarfExpressionMachineExecute_dup(machine);
      break;
    case DW_OP_and:
      success = FIRCLSDwarfExpressionMachineExecute_and(machine);
      break;
    case DW_OP_plus:
      success = FIRCLSDwarfExpressionMachineExecute_plus(machine);
      break;
    case DW_OP_swap:
      success = FIRCLSDwarfExpressionMachineExecute_swap(machine);
      break;
    case DW_OP_plus_uconst:
      success = FIRCLSDwarfExpressionMachineExecute_plus_uconst(machine);
      break;
    case DW_OP_ne:
      success = FIRCLSDwarfExpressionMachineExecute_ne(machine);
      break;
    case DW_OP_lit0:
    case DW_OP_lit1:
    case DW_OP_lit2:
    case DW_OP_lit3:
    case DW_OP_lit4:
    case DW_OP_lit5:
    case DW_OP_lit6:
    case DW_OP_lit7:
    case DW_OP_lit8:
    case DW_OP_lit9:
    case DW_OP_lit10:
    case DW_OP_lit11:
    case DW_OP_lit12:
    case DW_OP_lit13:
    case DW_OP_lit14:
    case DW_OP_lit15:
    case DW_OP_lit16:
    case DW_OP_lit17:
    case DW_OP_lit18:
    case DW_OP_lit19:
    case DW_OP_lit20:
    case DW_OP_lit21:
    case DW_OP_lit22:
    case DW_OP_lit23:
    case DW_OP_lit24:
    case DW_OP_lit25:
    case DW_OP_lit26:
    case DW_OP_lit27:
    case DW_OP_lit28:
    case DW_OP_lit29:
    case DW_OP_lit30:
    case DW_OP_lit31:
      success = FIRCLSDwarfExpressionMachineExecute_litN(machine, opcode);
      break;
    case DW_OP_breg0:
    case DW_OP_breg1:
    case DW_OP_breg2:
    case DW_OP_breg3:
    case DW_OP_breg4:
    case DW_OP_breg5:
    case DW_OP_breg6:
    case DW_OP_breg7:
    case DW_OP_breg8:
    case DW_OP_breg9:
    case DW_OP_breg10:
    case DW_OP_breg11:
    case DW_OP_breg12:
    case DW_OP_breg13:
    case DW_OP_breg14:
    case DW_OP_breg15:
    case DW_OP_breg16:
    case DW_OP_breg17:
    case DW_OP_breg18:
    case DW_OP_breg19:
    case DW_OP_breg20:
    case DW_OP_breg21:
    case DW_OP_breg22:
    case DW_OP_breg23:
    case DW_OP_breg24:
    case DW_OP_breg25:
    case DW_OP_breg26:
    case DW_OP_breg27:
    case DW_OP_breg28:
    case DW_OP_breg29:
    case DW_OP_breg30:
    case DW_OP_breg31:
      success = FIRCLSDwarfExpressionMachineExecute_bregN(machine, opcode);
      break;
    case DW_OP_deref_size:
      success = FIRCLSDwarfExpressionMachineExecute_deref_size(machine);
      break;
    default:
      FIRCLSSDKLog("Error: Unrecognized DWARF expression opcode 0x%x\n", opcode);
      return false;
  }

  return success;
}

#pragma mark -
#pragma mark Helpers
static intptr_t FIRCLSDwarfExpressionMachineStackPop(FIRCLSDwarfExpressionMachine *machine) {
  return FIRCLSDwarfExpressionStackPop(&machine->stack);
}

static bool FIRCLSDwarfExpressionMachineStackPush(FIRCLSDwarfExpressionMachine *machine,
                                                  intptr_t value) {
  return FIRCLSDwarfExpressionStackPush(&machine->stack, value);
}

#pragma mark -
#pragma mark Opcode Implementations
static bool FIRCLSDwarfExpressionMachineExecute_bregN(FIRCLSDwarfExpressionMachine *machine,
                                                      uint8_t opcode) {
  // find the register number, compute offset value, push
  const uint8_t regNum = opcode - DW_OP_breg0;

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: DW_OP_breg invalid register number\n");
    return false;
  }

  int64_t offset = FIRCLSParseLEB128AndAdvance(&machine->dataCursor);

  FIRCLSSDKLog("DW_OP_breg %d value %d\n", regNum, (int)offset);

  const intptr_t value =
      FIRCLSDwarfUnwindGetRegisterValue(machine->registers, regNum) + (intptr_t)offset;

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_deref(FIRCLSDwarfExpressionMachine *machine) {
  // pop stack, dereference, push result
  intptr_t value = FIRCLSDwarfExpressionMachineStackPop(machine);

  FIRCLSSDKLog("DW_OP_deref value %p\n", (void *)value);

  if (!FIRCLSReadMemory(value, &value, sizeof(value))) {
    FIRCLSSDKLog("Error: DW_OP_deref failed to read memory\n");
    return false;
  }

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_plus_uconst(FIRCLSDwarfExpressionMachine *machine) {
  // pop stack, add constant, push result
  intptr_t value = FIRCLSDwarfExpressionMachineStackPop(machine);

  value += FIRCLSParseULEB128AndAdvance(&machine->dataCursor);

  FIRCLSSDKLog("DW_OP_plus_uconst value %lu\n", value);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_and(FIRCLSDwarfExpressionMachine *machine) {
  FIRCLSSDKLog("DW_OP_plus_and\n");

  intptr_t value = FIRCLSDwarfExpressionMachineStackPop(machine);

  value = value & FIRCLSDwarfExpressionMachineStackPop(machine);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_plus(FIRCLSDwarfExpressionMachine *machine) {
  FIRCLSSDKLog("DW_OP_plus\n");

  intptr_t value = FIRCLSDwarfExpressionMachineStackPop(machine);

  value = value + FIRCLSDwarfExpressionMachineStackPop(machine);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_dup(FIRCLSDwarfExpressionMachine *machine) {
  // duplicate top of stack
  intptr_t value = FIRCLSDwarfExpressionStackPeek(&machine->stack);

  FIRCLSSDKLog("DW_OP_dup value %lu\n", value);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_swap(FIRCLSDwarfExpressionMachine *machine) {
  // swap top two values on the stack
  intptr_t valueA = FIRCLSDwarfExpressionMachineStackPop(machine);
  intptr_t valueB = FIRCLSDwarfExpressionMachineStackPop(machine);

  FIRCLSSDKLog("DW_OP_swap\n");

  if (!FIRCLSDwarfExpressionMachineStackPush(machine, valueA)) {
    return false;
  }

  return FIRCLSDwarfExpressionMachineStackPush(machine, valueB);
}

static bool FIRCLSDwarfExpressionMachineExecute_deref_size(FIRCLSDwarfExpressionMachine *machine) {
  // pop stack, dereference variable sized value, push result
  const void *address = (const void *)FIRCLSDwarfExpressionMachineStackPop(machine);
  const uint8_t readSize = FIRCLSParseUint8AndAdvance(&machine->dataCursor);
  intptr_t value = 0;

  FIRCLSSDKLog("DW_OP_deref_size %p size %u\n", address, readSize);

  switch (readSize) {
    case 1:
      value = FIRCLSParseUint8AndAdvance(&address);
      break;
    case 2:
      value = FIRCLSParseUint16AndAdvance(&address);
      break;
    case 4:
      value = FIRCLSParseUint32AndAdvance(&address);
      break;
    case 8:
      // this is a little funky, as an 8 here really doesn't make sense for 32-bit platforms
      value = (intptr_t)FIRCLSParseUint64AndAdvance(&address);
      break;
    default:
      FIRCLSSDKLog("Error: unrecognized DW_OP_deref_size argument %x\n", readSize);
      return false;
  }

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_ne(FIRCLSDwarfExpressionMachine *machine) {
  FIRCLSSDKLog("DW_OP_ne\n");

  intptr_t value = FIRCLSDwarfExpressionMachineStackPop(machine);

  value = value != FIRCLSDwarfExpressionMachineStackPop(machine);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

static bool FIRCLSDwarfExpressionMachineExecute_litN(FIRCLSDwarfExpressionMachine *machine,
                                                     uint8_t opcode) {
  const uint8_t value = opcode - DW_OP_lit0;

  FIRCLSSDKLog("DW_OP_lit %u\n", value);

  return FIRCLSDwarfExpressionMachineStackPush(machine, value);
}

#else
INJECT_STRIP_SYMBOL(dwarf_expression_machine)
#endif

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

#include "FIRCLSDwarfUnwind.h"
#include "FIRCLSDataParsing.h"
#include "FIRCLSDefines.h"
#include "FIRCLSDwarfExpressionMachine.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSUnwind_arch.h"
#include "FIRCLSUtility.h"
#include "dwarf.h"

#include <string.h>

#if CLS_DWARF_UNWINDING_SUPPORTED

#define FIRCLSDwarfLog(__FORMAT__, ...) FIRCLSSDKLog(__FORMAT__, ##__VA_ARGS__)

#define CLS_DWARF_EXPRESSION_STACK_SIZE (100)

#pragma mark Prototypes
static bool FIRCLSDwarfParseAndProcessAugmentation(DWARFCIERecord* record, const void** ptr);

#pragma mark - Record Parsing
bool FIRCLSDwarfParseCIERecord(DWARFCIERecord* cie, const void* ptr) {
  if (!cie || !ptr) {
    return false;
  }

  memset(cie, 0, sizeof(DWARFCIERecord));

  cie->length = FIRCLSParseRecordLengthAndAdvance(&ptr);
  if (cie->length == 0) {
    FIRCLSSDKLog("Error: CIE length invalid\n");
    return false;
  }

  // the length does not include the length field(s) themselves
  const void* endAddress = ptr + cie->length;

  if (FIRCLSParseUint32AndAdvance(&ptr) != DWARF_CIE_ID_CIE_FLAG) {
    FIRCLSSDKLog("Error: CIE flag not found\n");
  }

  cie->version = FIRCLSParseUint8AndAdvance(&ptr);
  if (cie->version != 1 && cie->version != 3) {
    FIRCLSSDKLog("Error: CIE version %u unsupported\n", cie->version);
  }

  cie->pointerEncoding = DW_EH_PE_absptr;
  cie->lsdaEncoding = DW_EH_PE_absptr;

  cie->augmentation = FIRCLSParseStringAndAdvance(&ptr);
  cie->codeAlignFactor = FIRCLSParseULEB128AndAdvance(&ptr);
  cie->dataAlignFactor = FIRCLSParseLEB128AndAdvance(&ptr);

  switch (cie->version) {
    case 1:
      cie->returnAddressRegister = FIRCLSParseUint8AndAdvance(&ptr);
      break;
    case 3:
      cie->returnAddressRegister = FIRCLSParseULEB128AndAdvance(&ptr);
      break;
    default:
      FIRCLSSDKLog("Error: CIE version %u unsupported\n", cie->version);
      return false;
  }

  if (!FIRCLSDwarfParseAndProcessAugmentation(cie, &ptr)) {
    return false;
  }

  cie->instructions.data = ptr;
  cie->instructions.length = (uint32_t)(endAddress - ptr);

  return true;
}

static bool FIRCLSDwarfParseAndProcessAugmentation(DWARFCIERecord* record, const void** ptr) {
  if (!record || !ptr) {
    return false;
  }

  if (!record->augmentation) {
    return false;
  }

  if (record->augmentation[0] == 0) {
    return true;
  }

  if (record->augmentation[0] != 'z') {
    FIRCLSSDKLog("Error: Unimplemented: augmentation string %s\n", record->augmentation);
    return false;
  }

  size_t stringLength = strlen(record->augmentation);

  uint64_t dataLength = FIRCLSParseULEB128AndAdvance(ptr);
  const void* ending = *ptr + dataLength;

  // start at 1 because we know the first character is a 'z'
  for (size_t i = 1; i < stringLength; ++i) {
    switch (record->augmentation[i]) {
      case 'L':
        // There is an LSDA pointer encoding present.  The actual address of the LSDA
        // is in the FDE
        record->lsdaEncoding = FIRCLSParseUint8AndAdvance(ptr);
        break;
      case 'R':
        // There is a pointer encoding present, used for all addresses in an FDE.
        record->pointerEncoding = FIRCLSParseUint8AndAdvance(ptr);
        break;
      case 'P':
        // Two arguments.  A pointer encoding, and a pointer to a personality function encoded
        // with that value.
        record->personalityEncoding = FIRCLSParseUint8AndAdvance(ptr);
        record->personalityFunction =
            FIRCLSParseAddressWithEncodingAndAdvance(ptr, record->personalityEncoding);
        if (record->personalityFunction == CLS_INVALID_ADDRESS) {
          FIRCLSSDKLog("Error: Found an invalid start address\n");
          return false;
        }
        break;
      case 'S':
        record->signalFrame = true;
        break;
      default:
        FIRCLSSDKLog("Error: Unhandled augmentation string entry %c\n", record->augmentation[i]);
        return false;
    }

    // small sanity check
    if (*ptr > ending) {
      return false;
    }
  }

  return true;
}

bool FIRCLSDwarfParseFDERecord(DWARFFDERecord* fdeRecord,
                               bool parseCIE,
                               DWARFCIERecord* cieRecord,
                               const void* ptr) {
  if (!fdeRecord || !cieRecord || !ptr) {
    return false;
  }

  fdeRecord->length = FIRCLSParseRecordLengthAndAdvance(&ptr);
  if (fdeRecord->length == 0) {
    FIRCLSSDKLog("Error: FDE has zero length\n");
    return false;
  }

  // length does not include length field
  const void* endAddress = ptr + fdeRecord->length;

  // According to the spec, this is 32/64 bit value, but libunwind always
  // parses this as a 32bit value.
  fdeRecord->cieOffset = FIRCLSParseUint32AndAdvance(&ptr);
  if (fdeRecord->cieOffset == 0) {
    FIRCLSSDKLog("Error: CIE offset invalid\n");
    return false;
  }

  if (parseCIE) {
    // The CIE offset is really weird. It appears to be an offset from the
    // beginning of its field. This isn't what the documentation says, but it is
    // a little ambigious. This is what DwarfParser.hpp does.
    // Note that we have to back up one sizeof(uint32_t), because we've advanced
    // by parsing the offset
    const void* ciePointer = ptr - fdeRecord->cieOffset - sizeof(uint32_t);
    if (!FIRCLSDwarfParseCIERecord(cieRecord, ciePointer)) {
      FIRCLSSDKLog("Error: Unable to parse CIE record\n");
      return false;
    }
  }

  if (!FIRCLSDwarfCIEIsValid(cieRecord)) {
    FIRCLSSDKLog("Error: CIE invalid\n");
    return false;
  }

  // the next field depends on the pointer encoding style used
  fdeRecord->startAddress =
      FIRCLSParseAddressWithEncodingAndAdvance(&ptr, cieRecord->pointerEncoding);
  if (fdeRecord->startAddress == CLS_INVALID_ADDRESS) {
    FIRCLSSDKLog("Error: Found an invalid start address\n");
    return false;
  }

  // Here's something weird too. The range is encoded as a "special" address, where only the value
  // is used, regardless of other pointer-encoding schemes.
  fdeRecord->rangeSize = FIRCLSParseAddressWithEncodingAndAdvance(
      &ptr, cieRecord->pointerEncoding & DW_EH_PE_VALUE_MASK);
  if (fdeRecord->rangeSize == CLS_INVALID_ADDRESS) {
    FIRCLSSDKLog("Error: Found an invalid address range\n");
    return false;
  }

  // Just skip over the section for now. The data here is only needed for personality functions,
  // which we don't need
  if (FIRCLSDwarfCIEHasAugmentationData(cieRecord)) {
    uintptr_t augmentationLength = (uintptr_t)FIRCLSParseULEB128AndAdvance(&ptr);

    ptr += augmentationLength;
  }

  fdeRecord->instructions.data = ptr;
  fdeRecord->instructions.length = (uint32_t)(endAddress - ptr);

  return true;
}

bool FIRCLSDwarfParseCFIFromFDERecord(FIRCLSDwarfCFIRecord* record, const void* ptr) {
  if (!record || !ptr) {
    return false;
  }

  return FIRCLSDwarfParseFDERecord(&record->fde, true, &record->cie, ptr);
}

bool FIRCLSDwarfParseCFIFromFDERecordOffset(FIRCLSDwarfCFIRecord* record,
                                            const void* ehFrame,
                                            uintptr_t fdeOffset) {
  if (!record || !ehFrame || (fdeOffset == 0)) {
    return false;
  }

  const void* ptr = ehFrame + fdeOffset;

  return FIRCLSDwarfParseCFIFromFDERecord(record, ptr);
}

#pragma mark - Properties
bool FIRCLSDwarfCIEIsValid(DWARFCIERecord* cie) {
  if (!cie) {
    return false;
  }

  if (cie->length == 0) {
    return false;
  }

  if (cie->version != 1 && cie->version != 3) {
    return false;
  }

  return true;
}

bool FIRCLSDwarfCIEHasAugmentationData(DWARFCIERecord* cie) {
  if (!cie) {
    return false;
  }

  if (!cie->augmentation) {
    return false;
  }

  return cie->augmentation[0] == 'z';
}

#pragma mark - Instructions

static bool FIRCLSDwarfParseAndExecute_set_loc(const void** cursor,
                                               DWARFCIERecord* cieRecord,
                                               intptr_t* codeOffset) {
  uintptr_t operand = FIRCLSParseAddressWithEncodingAndAdvance(cursor, cieRecord->pointerEncoding);

  *codeOffset = operand;

  FIRCLSDwarfLog("DW_CFA_set_loc %lu\n", operand);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_advance_loc1(const void** cursor,
                                                    DWARFCIERecord* cieRecord,
                                                    intptr_t* codeOffset) {
  int64_t offset = FIRCLSParseUint8AndAdvance(cursor) * cieRecord->codeAlignFactor;

  *codeOffset += offset;

  FIRCLSDwarfLog("DW_CFA_advance_loc1 %lld\n", offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_advance_loc2(const void** cursor,
                                                    DWARFCIERecord* cieRecord,
                                                    intptr_t* codeOffset) {
  int64_t offset = FIRCLSParseUint16AndAdvance(cursor) * cieRecord->codeAlignFactor;

  *codeOffset += offset;

  FIRCLSDwarfLog("DW_CFA_advance_loc2 %lld\n", offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_advance_loc4(const void** cursor,
                                                    DWARFCIERecord* cieRecord,
                                                    intptr_t* codeOffset) {
  int64_t offset = FIRCLSParseUint32AndAdvance(cursor) * cieRecord->codeAlignFactor;

  *codeOffset += offset;

  FIRCLSDwarfLog("DW_CFA_advance_loc4 %lld\n", offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_def_cfa(const void** cursor,
                                               DWARFCIERecord* cieRecord,
                                               FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_def_cfa register number\n");
    return false;
  }

  int64_t offset = FIRCLSParseULEB128AndAdvance(cursor);

  state->cfaRegister = regNum;
  state->cfaRegisterOffset = offset;

  FIRCLSDwarfLog("DW_CFA_def_cfa %llu, %lld\n", regNum, offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_def_cfa_register(const void** cursor,
                                                        DWARFCIERecord* cieRecord,
                                                        FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_def_cfa_register register number\n");
    return false;
  }

  state->cfaRegister = regNum;

  FIRCLSDwarfLog("DW_CFA_def_cfa_register %llu\n", regNum);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_def_cfa_offset(const void** cursor,
                                                      DWARFCIERecord* cieRecord,
                                                      FIRCLSDwarfState* state) {
  uint64_t offset = FIRCLSParseULEB128AndAdvance(cursor);

  state->cfaRegisterOffset = offset;

  FIRCLSDwarfLog("DW_CFA_def_cfa_offset %lld\n", offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_same_value(const void** cursor,
                                                  DWARFCIERecord* cieRecord,
                                                  FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_same_value register number\n");
    return false;
  }

  state->registers[regNum].location = FIRCLSDwarfRegisterUnused;

  FIRCLSDwarfLog("DW_CFA_same_value %llu\n", regNum);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_register(const void** cursor,
                                                DWARFCIERecord* cieRecord,
                                                FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_register number\n");
    return false;
  }

  uint64_t regValue = FIRCLSParseULEB128AndAdvance(cursor);

  if (regValue > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_register value\n");
    return false;
  }

  state->registers[regNum].location = FIRCLSDwarfRegisterInRegister;
  state->registers[regNum].value = regValue;

  FIRCLSDwarfLog("DW_CFA_register %llu %llu\n", regNum, regValue);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_expression(const void** cursor,
                                                  DWARFCIERecord* cieRecord,
                                                  FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_expression register number\n");
    return false;
  }

  state->registers[regNum].location = FIRCLSDwarfRegisterAtExpression;
  state->registers[regNum].value = (uintptr_t)*cursor;

  // read the length of the expression, and advance past it
  uint64_t length = FIRCLSParseULEB128AndAdvance(cursor);
  *cursor += length;

  FIRCLSDwarfLog("DW_CFA_expression %llu %llu\n", regNum, length);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_val_expression(const void** cursor,
                                                      DWARFCIERecord* cieRecord,
                                                      FIRCLSDwarfState* state) {
  uint64_t regNum = FIRCLSParseULEB128AndAdvance(cursor);

  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_val_expression register number\n");
    return false;
  }

  state->registers[regNum].location = FIRCLSDwarfRegisterIsExpression;
  state->registers[regNum].value = (uintptr_t)*cursor;

  // read the length of the expression, and advance past it
  uint64_t length = FIRCLSParseULEB128AndAdvance(cursor);
  *cursor += length;

  FIRCLSDwarfLog("DW_CFA_val_expression %llu %llu\n", regNum, length);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_def_cfa_expression(const void** cursor,
                                                          DWARFCIERecord* cieRecord,
                                                          FIRCLSDwarfState* state) {
  state->cfaRegister = CLS_DWARF_INVALID_REGISTER_NUM;
  state->cfaExpression = *cursor;

  // read the length of the expression, and advance past it
  uint64_t length = FIRCLSParseULEB128AndAdvance(cursor);
  *cursor += length;

  FIRCLSDwarfLog("DW_CFA_def_cfa_expression %llu\n", length);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_offset(const void** cursor,
                                              DWARFCIERecord* cieRecord,
                                              FIRCLSDwarfState* state,
                                              uint8_t regNum) {
  if (regNum > CLS_DWARF_MAX_REGISTER_NUM) {
    FIRCLSSDKLog("Error: Found an invalid DW_CFA_offset register number\n");
    return false;
  }

  int64_t offset = FIRCLSParseULEB128AndAdvance(cursor) * cieRecord->dataAlignFactor;

  state->registers[regNum].location = FIRCLSDwarfRegisterInCFA;
  state->registers[regNum].value = offset;

  FIRCLSDwarfLog("DW_CFA_offset %u, %lld\n", regNum, offset);

  return true;
}

static bool FIRCLSDwarfParseAndExecute_advance_loc(const void** cursor,
                                                   DWARFCIERecord* cieRecord,
                                                   FIRCLSDwarfState* state,
                                                   uint8_t delta,
                                                   intptr_t* codeOffset) {
  if (!FIRCLSIsValidPointer(codeOffset) || !FIRCLSIsValidPointer(cieRecord)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return false;
  }

  *codeOffset = delta * (intptr_t)cieRecord->codeAlignFactor;

  FIRCLSDwarfLog("DW_CFA_advance_loc %u\n", delta);

  return true;
}

static bool FIRCLSDwarfParseAndExecuteInstructionWithOperand(const void** cursor,
                                                             uint8_t instruction,
                                                             DWARFCIERecord* cieRecord,
                                                             FIRCLSDwarfState* state,
                                                             intptr_t* codeOffset) {
  uint8_t operand = instruction & DW_CFA_OPERAND_MASK;
  bool success = false;

  switch (instruction & DW_CFA_OPCODE_MASK) {
    case DW_CFA_offset:
      success = FIRCLSDwarfParseAndExecute_offset(cursor, cieRecord, state, operand);
      break;
    case DW_CFA_advance_loc:
      success =
          FIRCLSDwarfParseAndExecute_advance_loc(cursor, cieRecord, state, operand, codeOffset);
      break;
    case DW_CFA_restore:
      FIRCLSSDKLog("Error: Unimplemented DWARF instruction with operand 0x%x\n", instruction);
      break;
    default:
      FIRCLSSDKLog("Error: Unrecognized DWARF instruction 0x%x\n", instruction);
      break;
  }

  return success;
}

#pragma mark - Expressions
static bool FIRCLSDwarfEvalulateExpression(const void* cursor,
                                           const FIRCLSThreadContext* registers,
                                           intptr_t stackValue,
                                           intptr_t* result) {
  FIRCLSDwarfLog("starting at %p with initial value %lx\n", cursor, stackValue);

  if (!FIRCLSIsValidPointer(cursor) || !FIRCLSIsValidPointer(result)) {
    FIRCLSSDKLog("Error: inputs invalid\n");
    return false;
  }

  FIRCLSDwarfExpressionMachine machine;

  if (!FIRCLSDwarfExpressionMachineInit(&machine, cursor, registers, stackValue)) {
    FIRCLSSDKLog("Error: unable to init DWARF expression machine\n");
    return false;
  }

  if (!FIRCLSDwarfExpressionMachinePrepareForExecution(&machine)) {
    FIRCLSSDKLog("Error: unable to prepare for execution\n");
    return false;
  }

  while (!FIRCLSDwarfExpressionMachineIsFinished(&machine)) {
    if (!FIRCLSDwarfExpressionMachineExecuteNextOpcode(&machine)) {
      FIRCLSSDKLog("Error: failed to execute DWARF machine opcode\n");
      return false;
    }
  }

  if (!FIRCLSDwarfExpressionMachineGetResult(&machine, result)) {
    FIRCLSSDKLog("Error: failed to get DWARF expression result\n");
    return false;
  }

  FIRCLSDwarfLog("successfully computed expression result\n");

  return true;
}

#pragma mark - Execution
bool FIRCLSDwarfInstructionsEnumerate(DWARFInstructions* instructions,
                                      DWARFCIERecord* cieRecord,
                                      FIRCLSDwarfState* state,
                                      intptr_t pcOffset) {
  if (!instructions || !cieRecord || !state) {
    FIRCLSSDKLog("Error: inputs invalid\n");
    return false;
  }

  // This is a little bit of state that can't be put into the state structure, because
  // it is possible for instructions to push/pop state that does not affect this value.
  intptr_t codeOffset = 0;

  const void* cursor = instructions->data;
  const void* endAddress = cursor + instructions->length;

  FIRCLSDwarfLog("Running instructions from %p to %p\n", cursor, endAddress);

  // parse the instructions, as long as:
  // - our data pointer is still in range
  // - the pc offset is within the range of instructions that apply

  while ((cursor < endAddress) && (codeOffset < pcOffset)) {
    uint8_t instruction = FIRCLSParseUint8AndAdvance(&cursor);
    bool success = false;

    switch (instruction) {
      case DW_CFA_nop:
        FIRCLSDwarfLog("DW_CFA_nop\n");
        continue;
      case DW_CFA_set_loc:
        success = FIRCLSDwarfParseAndExecute_set_loc(&cursor, cieRecord, &codeOffset);
        break;
      case DW_CFA_advance_loc1:
        success = FIRCLSDwarfParseAndExecute_advance_loc1(&cursor, cieRecord, &codeOffset);
        break;
      case DW_CFA_advance_loc2:
        success = FIRCLSDwarfParseAndExecute_advance_loc2(&cursor, cieRecord, &codeOffset);
        break;
      case DW_CFA_advance_loc4:
        success = FIRCLSDwarfParseAndExecute_advance_loc4(&cursor, cieRecord, &codeOffset);
        break;
      case DW_CFA_def_cfa:
        success = FIRCLSDwarfParseAndExecute_def_cfa(&cursor, cieRecord, state);
        break;
      case DW_CFA_def_cfa_register:
        success = FIRCLSDwarfParseAndExecute_def_cfa_register(&cursor, cieRecord, state);
        break;
      case DW_CFA_def_cfa_offset:
        success = FIRCLSDwarfParseAndExecute_def_cfa_offset(&cursor, cieRecord, state);
        break;
      case DW_CFA_same_value:
        success = FIRCLSDwarfParseAndExecute_same_value(&cursor, cieRecord, state);
        break;
      case DW_CFA_register:
        success = FIRCLSDwarfParseAndExecute_register(&cursor, cieRecord, state);
        break;
      case DW_CFA_def_cfa_expression:
        success = FIRCLSDwarfParseAndExecute_def_cfa_expression(&cursor, cieRecord, state);
        break;
      case DW_CFA_expression:
        success = FIRCLSDwarfParseAndExecute_expression(&cursor, cieRecord, state);
        break;
      case DW_CFA_val_expression:
        success = FIRCLSDwarfParseAndExecute_val_expression(&cursor, cieRecord, state);
        break;
      case DW_CFA_offset_extended:
      case DW_CFA_restore_extended:
      case DW_CFA_undefined:
      case DW_CFA_remember_state:
      case DW_CFA_restore_state:
      case DW_CFA_offset_extended_sf:
      case DW_CFA_def_cfa_sf:
      case DW_CFA_def_cfa_offset_sf:
      case DW_CFA_val_offset:
      case DW_CFA_val_offset_sf:
      case DW_CFA_GNU_window_save:
      case DW_CFA_GNU_args_size:
      case DW_CFA_GNU_negative_offset_extended:
        FIRCLSSDKLog("Error: Unimplemented DWARF instruction 0x%x\n", instruction);
        return false;
      default:
        success = FIRCLSDwarfParseAndExecuteInstructionWithOperand(&cursor, instruction, cieRecord,
                                                                   state, &codeOffset);
        break;
    }

    if (!success) {
      FIRCLSSDKLog("Error: Failed to execute dwarf instruction 0x%x\n", instruction);
      return false;
    }
  }

  return true;
}

bool FIRCLSDwarfUnwindComputeRegisters(FIRCLSDwarfCFIRecord* record,
                                       FIRCLSThreadContext* registers) {
  if (!record || !registers) {
    return false;
  }

  // We need to run the dwarf instructions to compute our register values.
  // - initialize state
  // - run the CIE instructions
  // - run the FDE instructions
  // - grab the values

  FIRCLSDwarfState state;

  memset(&state, 0, sizeof(FIRCLSDwarfState));

  // We need to run all the instructions in the CIE record. So, pass in a large value for the pc
  // offset so we don't stop early.
  if (!FIRCLSDwarfInstructionsEnumerate(&record->cie.instructions, &record->cie, &state,
                                        INTPTR_MAX)) {
    FIRCLSSDKLog("Error: Unable to run CIE instructions\n");
    return false;
  }

  intptr_t pcOffset = FIRCLSThreadContextGetPC(registers) - record->fde.startAddress;
  if (pcOffset < 0) {
    FIRCLSSDKLog("Error: The FDE pcOffset value cannot be negative\n");
    return false;
  }

  if (!FIRCLSDwarfInstructionsEnumerate(&record->fde.instructions, &record->cie, &state,
                                        pcOffset)) {
    FIRCLSSDKLog("Error: Unable to run FDE instructions\n");
    return false;
  }

  uintptr_t cfaRegister = 0;

  if (!FIRCLSDwarfGetCFA(&state, registers, &cfaRegister)) {
    FIRCLSSDKLog("Error: failed to get CFA\n");
    return false;
  }

  if (!FIRCLSDwarfUnwindAssignRegisters(&state, registers, cfaRegister, registers)) {
    FIRCLSSDKLogError("Error: Unable to assign DWARF registers\n");
    return false;
  }

  return true;
}

bool FIRCLSDwarfUnwindAssignRegisters(const FIRCLSDwarfState* state,
                                      const FIRCLSThreadContext* registers,
                                      uintptr_t cfaRegister,
                                      FIRCLSThreadContext* outputRegisters) {
  if (!FIRCLSIsValidPointer(state) || !FIRCLSIsValidPointer(registers)) {
    FIRCLSSDKLogError("Error: input invalid\n");
    return false;
  }

  // make a copy, which we'll be changing
  FIRCLSThreadContext newThreadState = *registers;

  // loop through all the registers, so we can set their values
  for (size_t i = 0; i <= CLS_DWARF_MAX_REGISTER_NUM; ++i) {
    if (state->registers[i].location == FIRCLSDwarfRegisterUnused) {
      continue;
    }

    const uintptr_t value =
        FIRCLSDwarfGetSavedRegister(registers, cfaRegister, state->registers[i]);

    if (!FIRCLSDwarfUnwindSetRegisterValue(&newThreadState, i, value)) {
      FIRCLSSDKLog("Error: Unable to restore register value\n");
      return false;
    }
  }

  if (!FIRCLSDwarfUnwindSetRegisterValue(&newThreadState, CLS_DWARF_REG_SP, cfaRegister)) {
    FIRCLSSDKLog("Error: Unable to restore SP value\n");
    return false;
  }

  // sanity-check that things have changed
  if (FIRCLSDwarfCompareRegisters(registers, &newThreadState, CLS_DWARF_REG_SP)) {
    FIRCLSSDKLog("Error: Stack pointer hasn't changed\n");
    return false;
  }

  if (FIRCLSDwarfCompareRegisters(registers, &newThreadState, CLS_DWARF_REG_RETURN)) {
    FIRCLSSDKLog("Error: PC hasn't changed\n");
    return false;
  }

  // set our new value
  *outputRegisters = newThreadState;

  return true;
}

#pragma mark - Register Operations
bool FIRCLSDwarfCompareRegisters(const FIRCLSThreadContext* a,
                                 const FIRCLSThreadContext* b,
                                 uint64_t registerNum) {
  return FIRCLSDwarfUnwindGetRegisterValue(a, registerNum) ==
         FIRCLSDwarfUnwindGetRegisterValue(b, registerNum);
}

bool FIRCLSDwarfGetCFA(FIRCLSDwarfState* state,
                       const FIRCLSThreadContext* registers,
                       uintptr_t* cfa) {
  if (!FIRCLSIsValidPointer(state) || !FIRCLSIsValidPointer(registers) ||
      !FIRCLSIsValidPointer(cfa)) {
    FIRCLSSDKLog("Error: invalid input\n");
    return false;
  }

  if (state->cfaExpression) {
    if (!FIRCLSDwarfEvalulateExpression(state->cfaExpression, registers, 0, (intptr_t*)cfa)) {
      FIRCLSSDKLog("Error: failed to compute CFA expression\n");
      return false;
    }

    return true;
  }

  // libunwind checks that cfaRegister is not zero. This seems like a potential bug - why couldn't
  // it be zero?

  *cfa = FIRCLSDwarfUnwindGetRegisterValue(registers, state->cfaRegister) +
         (uintptr_t)state->cfaRegisterOffset;

  return true;
}

uintptr_t FIRCLSDwarfGetSavedRegister(const FIRCLSThreadContext* registers,
                                      uintptr_t cfaRegister,
                                      FIRCLSDwarfRegister dRegister) {
  intptr_t result = 0;

  FIRCLSDwarfLog("Getting register %x\n", dRegister.location);

  switch (dRegister.location) {
    case FIRCLSDwarfRegisterInCFA: {
      const uintptr_t address = cfaRegister + (uintptr_t)dRegister.value;

      if (!FIRCLSReadMemory(address, &result, sizeof(result))) {
        FIRCLSSDKLog("Error: Unable to read CFA value\n");
        return 0;
      }
    }
      return result;
    case FIRCLSDwarfRegisterInRegister:
      return FIRCLSDwarfUnwindGetRegisterValue(registers, dRegister.value);
    case FIRCLSDwarfRegisterOffsetFromCFA:
      FIRCLSSDKLog("Error: OffsetFromCFA unhandled\n");
      break;
    case FIRCLSDwarfRegisterAtExpression:
      if (!FIRCLSDwarfEvalulateExpression((void*)dRegister.value, registers, cfaRegister,
                                          &result)) {
        FIRCLSSDKLog("Error: unable to evaluate expression\n");
        return 0;
      }

      if (!FIRCLSReadMemory(result, &result, sizeof(result))) {
        FIRCLSSDKLog("Error: Unable to read memory computed from expression\n");
        return 0;
      }

      return result;
    case FIRCLSDwarfRegisterIsExpression:
      if (!FIRCLSDwarfEvalulateExpression((void*)dRegister.value, registers, cfaRegister,
                                          &result)) {
        FIRCLSSDKLog("Error: unable to evaluate expression\n");
        return 0;
      }

      return result;
    default:
      FIRCLSSDKLog("Error: Unrecognized register save location 0x%x\n", dRegister.location);
      break;
  }

  return 0;
}

#if DEBUG
#pragma mark - Debugging
void FIRCLSCFIRecordShow(FIRCLSDwarfCFIRecord* record) {
  if (!record) {
    FIRCLSSDKLog("Error: CFI record: null\n");
    return;
  }

  FIRCLSCIERecordShow(&record->cie);
  FIRCLSFDERecordShow(&record->fde, &record->cie);
}

void FIRCLSCIERecordShow(DWARFCIERecord* record) {
  if (!record) {
    FIRCLSSDKLog("Error: CIE: null\n");
    return;
  }

  FIRCLSSDKLog("CIE:\n");
  FIRCLSSDKLog("       length: %llu\n", record->length);
  FIRCLSSDKLog("      version: %u\n", record->version);
  FIRCLSSDKLog(" augmentation: %s\n", record->augmentation);
  FIRCLSSDKLog("      EH Data: 0x%04lx\n", record->ehData);
  FIRCLSSDKLog("LSDA encoding: 0x%02x\n", record->lsdaEncoding);
  FIRCLSSDKLog("  personality: 0x%lx\n", record->personalityFunction);

  FIRCLSDwarfPointerEncodingShow("     encoding", record->pointerEncoding);
  FIRCLSDwarfPointerEncodingShow("   P encoding", record->personalityEncoding);

  FIRCLSSDKLog("   code align: %llu\n", record->codeAlignFactor);
  FIRCLSSDKLog("   data align: %lld\n", record->dataAlignFactor);
  FIRCLSSDKLog("  RA register: %llu\n", record->returnAddressRegister);

  FIRCLSDwarfInstructionsShow(&record->instructions, record);
}

void FIRCLSFDERecordShow(DWARFFDERecord* record, DWARFCIERecord* cie) {
  if (!record) {
    FIRCLSSDKLog("FDE: null\n");
    return;
  }

  FIRCLSSDKLog("FDE:\n");
  FIRCLSSDKLog("      length: %llu\n", record->length);
  FIRCLSSDKLog("  CIE offset: %llu\n", record->cieOffset);
  FIRCLSSDKLog("  start addr: 0x%lx\n", record->startAddress);
  FIRCLSSDKLog("       range: %lu\n", record->rangeSize);

  FIRCLSDwarfInstructionsShow(&record->instructions, cie);
}

void FIRCLSDwarfPointerEncodingShow(const char* leadString, uint8_t encoding) {
  if (encoding == DW_EH_PE_omit) {
    FIRCLSSDKLog("%s: 0x%02x (omit)\n", leadString, encoding);
  } else {
    const char* peValue = "";
    const char* peOffset = "";

    switch (encoding & DW_EH_PE_VALUE_MASK) {
      case DW_EH_PE_absptr:
        peValue = "DW_EH_PE_absptr";
        break;
      case DW_EH_PE_uleb128:
        peValue = "DW_EH_PE_uleb128";
        break;
      case DW_EH_PE_udata2:
        peValue = "DW_EH_PE_udata2";
        break;
      case DW_EH_PE_udata4:
        peValue = "DW_EH_PE_udata4";
        break;
      case DW_EH_PE_udata8:
        peValue = "DW_EH_PE_udata8";
        break;
      case DW_EH_PE_signed:
        peValue = "DW_EH_PE_signed";
        break;
      case DW_EH_PE_sleb128:
        peValue = "DW_EH_PE_sleb128";
        break;
      case DW_EH_PE_sdata2:
        peValue = "DW_EH_PE_sdata2";
        break;
      case DW_EH_PE_sdata4:
        peValue = "DW_EH_PE_sdata4";
        break;
      case DW_EH_PE_sdata8:
        peValue = "DW_EH_PE_sdata8";
        break;
      default:
        peValue = "unknown";
        break;
    }

    switch (encoding & DW_EH_PE_RELATIVE_OFFSET_MASK) {
      case DW_EH_PE_absptr:
        break;
      case DW_EH_PE_pcrel:
        peOffset = " + DW_EH_PE_pcrel";
        break;
      case DW_EH_PE_textrel:
        peOffset = " + DW_EH_PE_textrel";
        break;
      case DW_EH_PE_datarel:
        peOffset = " + DW_EH_PE_datarel";
        break;
      case DW_EH_PE_funcrel:
        peOffset = " + DW_EH_PE_funcrel";
        break;
      case DW_EH_PE_aligned:
        peOffset = " + DW_EH_PE_aligned";
        break;
      case DW_EH_PE_indirect:
        peOffset = " + DW_EH_PE_indirect";
        break;
      default:
        break;
    }

    FIRCLSSDKLog("%s: 0x%02x (%s%s)\n", leadString, encoding, peValue, peOffset);
  }
}

void FIRCLSDwarfInstructionsShow(DWARFInstructions* instructions, DWARFCIERecord* cie) {
  if (!instructions) {
    FIRCLSSDKLog("Error: Instructions null\n");
  }

  FIRCLSDwarfState state;

  memset(&state, 0, sizeof(FIRCLSDwarfState));

  FIRCLSDwarfInstructionsEnumerate(instructions, cie, &state, -1);
}

#endif

#else
INJECT_STRIP_SYMBOL(dwarf_unwind)
#endif

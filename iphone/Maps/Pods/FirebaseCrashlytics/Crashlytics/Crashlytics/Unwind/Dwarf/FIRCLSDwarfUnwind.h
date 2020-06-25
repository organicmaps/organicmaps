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
#include <stdint.h>
#include <sys/types.h>

#include "FIRCLSDwarfUnwindRegisters.h"
#include "FIRCLSThreadState.h"

#if CLS_DWARF_UNWINDING_SUPPORTED

#pragma mark Structures
typedef struct {
  uint32_t length;
  const void* data;
} DWARFInstructions;

typedef struct {
  uint64_t length;
  uint8_t version;
  uintptr_t ehData;  // 8 bytes for 64-bit architectures, 4 bytes for 32
  const char* augmentation;
  uint8_t pointerEncoding;
  uint8_t lsdaEncoding;
  uint8_t personalityEncoding;
  uintptr_t personalityFunction;
  uint64_t codeAlignFactor;
  int64_t dataAlignFactor;
  uint64_t returnAddressRegister;  // is 64 bits enough for this value?
  bool signalFrame;

  DWARFInstructions instructions;
} DWARFCIERecord;

typedef struct {
  uint64_t length;
  uint64_t cieOffset;  // also an arch-specific size
  uintptr_t startAddress;
  uintptr_t rangeSize;

  DWARFInstructions instructions;
} DWARFFDERecord;

typedef struct {
  DWARFCIERecord cie;
  DWARFFDERecord fde;
} FIRCLSDwarfCFIRecord;

typedef enum {
  FIRCLSDwarfRegisterUnused = 0,
  FIRCLSDwarfRegisterInCFA,
  FIRCLSDwarfRegisterOffsetFromCFA,
  FIRCLSDwarfRegisterInRegister,
  FIRCLSDwarfRegisterAtExpression,
  FIRCLSDwarfRegisterIsExpression
} FIRCLSDwarfRegisterLocation;

typedef struct {
  FIRCLSDwarfRegisterLocation location;
  uint64_t value;
} FIRCLSDwarfRegister;

typedef struct {
  uint64_t cfaRegister;
  int64_t cfaRegisterOffset;
  const void* cfaExpression;
  uint32_t spArgSize;

  FIRCLSDwarfRegister registers[CLS_DWARF_MAX_REGISTER_NUM + 1];
} FIRCLSDwarfState;

__BEGIN_DECLS

#pragma mark - Parsing
bool FIRCLSDwarfParseCIERecord(DWARFCIERecord* cie, const void* ptr);
bool FIRCLSDwarfParseFDERecord(DWARFFDERecord* fdeRecord,
                               bool parseCIE,
                               DWARFCIERecord* cieRecord,
                               const void* ptr);
bool FIRCLSDwarfParseCFIFromFDERecord(FIRCLSDwarfCFIRecord* record, const void* ptr);
bool FIRCLSDwarfParseCFIFromFDERecordOffset(FIRCLSDwarfCFIRecord* record,
                                            const void* ehFrame,
                                            uintptr_t fdeOffset);

#pragma mark - Properties
bool FIRCLSDwarfCIEIsValid(DWARFCIERecord* cie);
bool FIRCLSDwarfCIEHasAugmentationData(DWARFCIERecord* cie);

#pragma mark - Execution
bool FIRCLSDwarfInstructionsEnumerate(DWARFInstructions* instructions,
                                      DWARFCIERecord* cieRecord,
                                      FIRCLSDwarfState* state,
                                      intptr_t pcOffset);
bool FIRCLSDwarfUnwindComputeRegisters(FIRCLSDwarfCFIRecord* record,
                                       FIRCLSThreadContext* registers);
bool FIRCLSDwarfUnwindAssignRegisters(const FIRCLSDwarfState* state,
                                      const FIRCLSThreadContext* registers,
                                      uintptr_t cfaRegister,
                                      FIRCLSThreadContext* outputRegisters);

#pragma mark - Register Operations
bool FIRCLSDwarfCompareRegisters(const FIRCLSThreadContext* a,
                                 const FIRCLSThreadContext* b,
                                 uint64_t registerNum);

bool FIRCLSDwarfGetCFA(FIRCLSDwarfState* state,
                       const FIRCLSThreadContext* registers,
                       uintptr_t* cfa);
uintptr_t FIRCLSDwarfGetSavedRegister(const FIRCLSThreadContext* registers,
                                      uintptr_t cfaRegister,
                                      FIRCLSDwarfRegister dRegister);

#if DEBUG
#pragma mark - Debugging
void FIRCLSCFIRecordShow(FIRCLSDwarfCFIRecord* record);
void FIRCLSCIERecordShow(DWARFCIERecord* record);
void FIRCLSFDERecordShow(DWARFFDERecord* record, DWARFCIERecord* cie);
void FIRCLSDwarfPointerEncodingShow(const char* leadString, uint8_t encoding);
void FIRCLSDwarfInstructionsShow(DWARFInstructions* instructions, DWARFCIERecord* cie);
#endif

__END_DECLS

#endif

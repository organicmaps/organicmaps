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

#include "FIRCLSFeatures.h"
#include "FIRCLSThreadState.h"

#define CLS_DWARF_EXPRESSION_STACK_SIZE (100)

#if CLS_DWARF_UNWINDING_SUPPORTED

typedef struct {
  intptr_t buffer[CLS_DWARF_EXPRESSION_STACK_SIZE];
  intptr_t *pointer;
} FIRCLSDwarfExpressionStack;

typedef struct {
  FIRCLSDwarfExpressionStack stack;
  const void *dataCursor;
  const void *endAddress;
  const FIRCLSThreadContext *registers;
} FIRCLSDwarfExpressionMachine;

void FIRCLSDwarfExpressionStackInit(FIRCLSDwarfExpressionStack *stack);
bool FIRCLSDwarfExpressionStackIsValid(FIRCLSDwarfExpressionStack *stack);
bool FIRCLSDwarfExpressionStackPush(FIRCLSDwarfExpressionStack *stack, intptr_t value);
intptr_t FIRCLSDwarfExpressionStackPeek(FIRCLSDwarfExpressionStack *stack);
intptr_t FIRCLSDwarfExpressionStackPop(FIRCLSDwarfExpressionStack *stack);

bool FIRCLSDwarfExpressionMachineInit(FIRCLSDwarfExpressionMachine *machine,
                                      const void *cursor,
                                      const FIRCLSThreadContext *registers,
                                      intptr_t stackValue);
bool FIRCLSDwarfExpressionMachinePrepareForExecution(FIRCLSDwarfExpressionMachine *machine);
bool FIRCLSDwarfExpressionMachineIsFinished(FIRCLSDwarfExpressionMachine *machine);
bool FIRCLSDwarfExpressionMachineGetResult(FIRCLSDwarfExpressionMachine *machine, intptr_t *result);

bool FIRCLSDwarfExpressionMachineExecuteNextOpcode(FIRCLSDwarfExpressionMachine *machine);

#endif

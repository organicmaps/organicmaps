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
#include "FIRCLSFile.h"

#include <signal.h>
#include <stdbool.h>

#define FIRCLSSignalCount (7)

// per man sigaltstack, MINSIGSTKSZ is the minimum *overhead* needed to support
// a signal stack.  The actual stack size must be larger.  Let's pick the recommended
// size.
#if CLS_USE_SIGALTSTACK
#define CLS_SIGNAL_HANDLER_STACK_SIZE (SIGSTKSZ * 2)
#else
#define CLS_SIGNAL_HANDLER_STACK_SIZE 0
#endif

typedef struct {
  const char* path;
  struct sigaction originalActions[FIRCLSSignalCount];

#if CLS_USE_SIGALTSTACK
  stack_t originalStack;
#endif
} FIRCLSSignalReadContext;

void FIRCLSSignalInitialize(FIRCLSSignalReadContext* roContext);
void FIRCLSSignalCheckHandlers(void);

void FIRCLSSignalSafeRemoveHandlers(bool includingAbort);
bool FIRCLSSignalSafeInstallPreexistingHandlers(FIRCLSSignalReadContext* roContext);

void FIRCLSSignalNameLookup(int number, int code, const char** name, const char** codeName);

void FIRCLSSignalEnumerateHandledSignals(void (^block)(int idx, int signal));

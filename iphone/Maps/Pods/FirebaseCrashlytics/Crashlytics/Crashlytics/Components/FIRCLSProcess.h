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

#include <mach/mach.h>
#include <stdbool.h>

#include "FIRCLSFile.h"

typedef struct {
  // task info
  mach_port_t task;

  // thread stuff
  thread_t thisThread;
  thread_t crashedThread;
  thread_act_array_t threads;
  mach_msg_type_number_t threadCount;
  void *uapVoid;  // current thread state
} FIRCLSProcess;

bool FIRCLSProcessInit(FIRCLSProcess *process, thread_t crashedThread, void *uapVoid);
bool FIRCLSProcessDestroy(FIRCLSProcess *process);
bool FIRCLSProcessDebuggerAttached(void);

bool FIRCLSProcessSuspendAllOtherThreads(FIRCLSProcess *process);
bool FIRCLSProcessResumeAllOtherThreads(FIRCLSProcess *process);

void FIRCLSProcessRecordThreadNames(FIRCLSProcess *process, FIRCLSFile *file);
void FIRCLSProcessRecordDispatchQueueNames(FIRCLSProcess *process, FIRCLSFile *file);
bool FIRCLSProcessRecordAllThreads(FIRCLSProcess *process, FIRCLSFile *file);
void FIRCLSProcessRecordStats(FIRCLSProcess *process, FIRCLSFile *file);
void FIRCLSProcessRecordRuntimeInfo(FIRCLSProcess *process, FIRCLSFile *file);

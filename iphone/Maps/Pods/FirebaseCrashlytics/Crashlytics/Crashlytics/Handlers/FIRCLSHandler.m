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

#include "FIRCLSHandler.h"

#include "FIRCLSCrashedMarkerFile.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSHost.h"
#include "FIRCLSProcess.h"
#include "FIRCLSUtility.h"

#import "FIRCLSReportManager_Private.h"

void FIRCLSHandler(FIRCLSFile* file, thread_t crashedThread, void* uapVoid) {
  FIRCLSProcess process;

  FIRCLSProcessInit(&process, crashedThread, uapVoid);

  FIRCLSProcessSuspendAllOtherThreads(&process);

  FIRCLSProcessRecordAllThreads(&process, file);

  FIRCLSProcessRecordRuntimeInfo(&process, file);
  // Get dispatch queue and thread names. Note that getting the thread names
  // can hang, so let's do that last
  FIRCLSProcessRecordDispatchQueueNames(&process, file);
  FIRCLSProcessRecordThreadNames(&process, file);

  // this stuff isn't super important, but we can try
  FIRCLSProcessRecordStats(&process, file);
  FIRCLSHostWriteDiskUsage(file);

  // This is the first common point where various crash handlers call into
  // Store a crash file marker to indicate that a crash has occured
  FIRCLSCreateCrashedMarkerFile();

  FIRCLSProcessResumeAllOtherThreads(&process);

  // clean up after ourselves
  FIRCLSProcessDestroy(&process);
}

void FIRCLSHandlerAttemptImmediateDelivery(void) {
  // now, attempt to relay the event to the delegate
  FIRCLSReportManager* manager = (__bridge id)_firclsContext.readonly->delegate;

  if ([manager respondsToSelector:@selector(potentiallySubmittableCrashOccurred)]) {
    [manager potentiallySubmittableCrashOccurred];
  }
}

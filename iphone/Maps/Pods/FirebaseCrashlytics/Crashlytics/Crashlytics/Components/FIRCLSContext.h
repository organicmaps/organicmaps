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

#include "FIRCLSAllocate.h"
#include "FIRCLSBinaryImage.h"
#include "FIRCLSException.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSHost.h"
#include "FIRCLSInternalLogging.h"
#include "FIRCLSMachException.h"
#include "FIRCLSSignal.h"
#include "FIRCLSUserLogging.h"

#include <dispatch/dispatch.h>
#include <stdbool.h>

// The purpose of the crash context is to hold values that absolutely must be read and/or written at
// crash time.  For robustness against memory corruption, they are protected with guard pages.
// Further, the context is seperated into read-only and read-write sections.

__BEGIN_DECLS

#ifdef __OBJC__
@class FIRCLSInternalReport;
@class FIRCLSSettings;
@class FIRCLSInstallIdentifierModel;
@class FIRCLSFileManager;
#endif

typedef struct {
  volatile bool initialized;
  volatile bool debuggerAttached;
  const char* previouslyCrashedFileFullPath;
  const char* logPath;
#if CLS_USE_SIGALTSTACK
  void* signalStack;
#endif
#if CLS_MACH_EXCEPTION_SUPPORTED
  void* machStack;
#endif
  void* delegate;
  void* callbackDelegate;

  FIRCLSBinaryImageReadOnlyContext binaryimage;
  FIRCLSExceptionReadOnlyContext exception;
  FIRCLSHostReadOnlyContext host;
  FIRCLSSignalReadContext signal;
#if CLS_MACH_EXCEPTION_SUPPORTED
  FIRCLSMachExceptionReadContext machException;
#endif
  FIRCLSUserLoggingReadOnlyContext logging;
} FIRCLSReadOnlyContext;

typedef struct {
  FIRCLSInternalLoggingWritableContext internalLogging;
  volatile bool crashOccurred;
  FIRCLSBinaryImageReadWriteContext binaryImage;
  FIRCLSUserLoggingWritableContext logging;
  FIRCLSExceptionWritableContext exception;
} FIRCLSReadWriteContext;

typedef struct {
  FIRCLSReadOnlyContext* readonly;
  FIRCLSReadWriteContext* writable;
  FIRCLSAllocatorRef allocator;
} FIRCLSContext;

typedef struct {
  void* delegate;
  const char* customBundleId;
  const char* rootPath;
  const char* previouslyCrashedFileRootPath;
  const char* sessionId;
  const char* installId;
  const char* betaToken;
#if CLS_MACH_EXCEPTION_SUPPORTED
  exception_mask_t machExceptionMask;
#endif
  bool errorsEnabled;
  bool customExceptionsEnabled;
  uint32_t maxCustomExceptions;
  uint32_t maxErrorLogSize;
  uint32_t maxLogSize;
  uint32_t maxKeyValues;
} FIRCLSContextInitData;

#ifdef __OBJC__
bool FIRCLSContextInitialize(FIRCLSInternalReport* report,
                             FIRCLSSettings* settings,
                             FIRCLSInstallIdentifierModel* installIDModel,
                             FIRCLSFileManager* fileManager);

// Re-writes the metadata file on the current thread
void FIRCLSContextUpdateMetadata(FIRCLSInternalReport* report,
                                 FIRCLSSettings* settings,
                                 FIRCLSInstallIdentifierModel* installIDModel,
                                 FIRCLSFileManager* fileManager);
#endif

void FIRCLSContextBaseInit(void);
void FIRCLSContextBaseDeinit(void);

bool FIRCLSContextIsInitialized(void);
bool FIRCLSContextHasCrashed(void);
void FIRCLSContextMarkHasCrashed(void);
bool FIRCLSContextMarkAndCheckIfCrashed(void);

__END_DECLS

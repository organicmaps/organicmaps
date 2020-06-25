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

#include <stdint.h>
#include <sys/cdefs.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
@class FIRStackFrame;
@class FIRExceptionModel;
#endif

#define CLS_EXCEPTION_STRING_LENGTH_MAX (1024 * 16)

typedef enum {
  FIRCLSExceptionTypeUnknown = 0,
  FIRCLSExceptionTypeObjectiveC = 1,
  FIRCLSExceptionTypeCpp = 2,
  // 3 was FIRCLSExceptionTypeJavascript
  // Keeping these numbers the same just to be safe
  FIRCLSExceptionTypeCustom = 4
} FIRCLSExceptionType;

typedef struct {
  const char* path;

  void (*originalTerminateHandler)(void);

#if !TARGET_OS_IPHONE
  void* originalNSApplicationReportException;
#endif

  uint32_t maxCustomExceptions;
} FIRCLSExceptionReadOnlyContext;

typedef struct {
  uint32_t customExceptionCount;
} FIRCLSExceptionWritableContext;

__BEGIN_DECLS

void FIRCLSExceptionInitialize(FIRCLSExceptionReadOnlyContext* roContext,
                               FIRCLSExceptionWritableContext* rwContext,
                               void* delegate);
void FIRCLSExceptionCheckHandlers(void* delegate);

void FIRCLSExceptionRaiseTestObjCException(void) __attribute((noreturn));
void FIRCLSExceptionRaiseTestCppException(void) __attribute((noreturn));

#ifdef __OBJC__
void FIRCLSExceptionRecordModel(FIRExceptionModel* exceptionModel);
void FIRCLSExceptionRecordNSException(NSException* exception);
void FIRCLSExceptionRecord(FIRCLSExceptionType type,
                           const char* name,
                           const char* reason,
                           NSArray<FIRStackFrame*>* frames,
                           BOOL attemptDelivery);
#endif

__END_DECLS

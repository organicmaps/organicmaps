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

#include <stdio.h>

#if __OBJC__
#import "FIRCLSLogger.h"
#define FIRCLSDeveloperLog(label, __FORMAT__, ...) \
  FIRCLSDebugLog(@"[" label "] " __FORMAT__, ##__VA_ARGS__);
#endif

typedef enum {
  FIRCLSInternalLogLevelUnknown = 0,
  FIRCLSInternalLogLevelDebug = 1,
  FIRCLSInternalLogLevelInfo = 2,
  FIRCLSInternalLogLevelWarn = 3,
  FIRCLSInternalLogLevelError = 4
} FIRCLSInternalLogLevel;

typedef struct {
  int logFd;
  FIRCLSInternalLogLevel logLevel;
} FIRCLSInternalLoggingWritableContext;

#define FIRCLSSDKLogDebug(__FORMAT__, ...)                                                 \
  FIRCLSSDKFileLog(FIRCLSInternalLogLevelDebug, "DEBUG [%s:%d] " __FORMAT__, __FUNCTION__, \
                   __LINE__, ##__VA_ARGS__)
#define FIRCLSSDKLogInfo(__FORMAT__, ...)                                                 \
  FIRCLSSDKFileLog(FIRCLSInternalLogLevelInfo, "INFO  [%s:%d] " __FORMAT__, __FUNCTION__, \
                   __LINE__, ##__VA_ARGS__)
#define FIRCLSSDKLogWarn(__FORMAT__, ...)                                                 \
  FIRCLSSDKFileLog(FIRCLSInternalLogLevelWarn, "WARN  [%s:%d] " __FORMAT__, __FUNCTION__, \
                   __LINE__, ##__VA_ARGS__)
#define FIRCLSSDKLogError(__FORMAT__, ...)                                                 \
  FIRCLSSDKFileLog(FIRCLSInternalLogLevelError, "ERROR [%s:%d] " __FORMAT__, __FUNCTION__, \
                   __LINE__, ##__VA_ARGS__)

#define FIRCLSSDKLog FIRCLSSDKLogWarn

__BEGIN_DECLS

void FIRCLSSDKFileLog(FIRCLSInternalLogLevel level, const char* format, ...) __printflike(2, 3);

__END_DECLS

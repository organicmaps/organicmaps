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

#import "FIRCLSLogger.h"

#import <FirebaseCore/FIRLogger.h>

FIRLoggerService kFIRLoggerCrashlytics = @"[Firebase/Crashlytics]";

NSString *const CrashlyticsMessageCode = @"I-CLS000000";

void FIRCLSDebugLog(NSString *message, ...) {
  va_list args_ptr;
  va_start(args_ptr, message);
  FIRLogBasic(FIRLoggerLevelDebug, kFIRLoggerCrashlytics, CrashlyticsMessageCode, message,
              args_ptr);
  va_end(args_ptr);
}

void FIRCLSInfoLog(NSString *message, ...) {
  va_list args_ptr;
  va_start(args_ptr, message);
  FIRLogBasic(FIRLoggerLevelInfo, kFIRLoggerCrashlytics, CrashlyticsMessageCode, message, args_ptr);
  va_end(args_ptr);
}

void FIRCLSWarningLog(NSString *message, ...) {
  va_list args_ptr;
  va_start(args_ptr, message);
  FIRLogBasic(FIRLoggerLevelWarning, kFIRLoggerCrashlytics, CrashlyticsMessageCode, message,
              args_ptr);
  va_end(args_ptr);
}

void FIRCLSErrorLog(NSString *message, ...) {
  va_list args_ptr;
  va_start(args_ptr, message);
  FIRLogBasic(FIRLoggerLevelError, kFIRLoggerCrashlytics, CrashlyticsMessageCode, message,
              args_ptr);
  va_end(args_ptr);
}

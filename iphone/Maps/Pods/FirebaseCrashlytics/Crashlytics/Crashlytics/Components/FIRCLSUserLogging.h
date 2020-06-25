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

#include "FIRCLSFile.h"

__BEGIN_DECLS

#ifdef __OBJC__
extern NSString* const FIRCLSStartTimeKey;
extern NSString* const FIRCLSFirstRunloopTurnTimeKey;
extern NSString* const FIRCLSInBackgroundKey;
#if TARGET_OS_IPHONE
extern NSString* const FIRCLSDeviceOrientationKey;
extern NSString* const FIRCLSUIOrientationKey;
#endif
extern NSString* const FIRCLSUserIdentifierKey;
extern NSString* const FIRCLSUserNameKey;
extern NSString* const FIRCLSUserEmailKey;
extern NSString* const FIRCLSDevelopmentPlatformNameKey;
extern NSString* const FIRCLSDevelopmentPlatformVersionKey;
#endif

extern const uint32_t FIRCLSUserLoggingMaxKVEntries;

typedef struct {
  const char* incrementalPath;
  const char* compactedPath;

  uint32_t maxIncrementalCount;
  uint32_t maxCount;
} FIRCLSUserLoggingKVStorage;

typedef struct {
  const char* aPath;
  const char* bPath;
  uint32_t maxSize;
  uint32_t maxEntries;
  bool restrictBySize;
  uint32_t* entryCount;
} FIRCLSUserLoggingABStorage;

typedef struct {
  FIRCLSUserLoggingKVStorage userKVStorage;
  FIRCLSUserLoggingKVStorage internalKVStorage;

  FIRCLSUserLoggingABStorage logStorage;
  FIRCLSUserLoggingABStorage errorStorage;
  FIRCLSUserLoggingABStorage customExceptionStorage;
} FIRCLSUserLoggingReadOnlyContext;

typedef struct {
  const char* activeUserLogPath;
  const char* activeErrorLogPath;
  const char* activeCustomExceptionPath;
  uint32_t userKVCount;
  uint32_t internalKVCount;
  uint32_t errorsCount;
} FIRCLSUserLoggingWritableContext;

void FIRCLSUserLoggingInit(FIRCLSUserLoggingReadOnlyContext* roContext,
                           FIRCLSUserLoggingWritableContext* rwContext);

#ifdef __OBJC__
void FIRCLSUserLoggingRecordUserKeyValue(NSString* key, id value);
void FIRCLSUserLoggingRecordInternalKeyValue(NSString* key, id value);
void FIRCLSUserLoggingWriteInternalKeyValue(NSString* key, NSString* value);

void FIRCLSUserLoggingRecordError(NSError* error, NSDictionary<NSString*, id>* additionalUserInfo);

NSDictionary* FIRCLSUserLoggingGetCompactedKVEntries(FIRCLSUserLoggingKVStorage* storage,
                                                     bool decodeHex);
void FIRCLSUserLoggingCompactKVEntries(FIRCLSUserLoggingKVStorage* storage);

void FIRCLSUserLoggingRecordKeyValue(NSString* key,
                                     id value,
                                     FIRCLSUserLoggingKVStorage* storage,
                                     uint32_t* counter);

void FIRCLSUserLoggingWriteAndCheckABFiles(FIRCLSUserLoggingABStorage* storage,
                                           const char** activePath,
                                           void (^openedFileBlock)(FIRCLSFile* file));

NSArray* FIRCLSUserLoggingStoredKeyValues(const char* path);

OBJC_EXTERN void FIRCLSLog(NSString* format, ...) NS_FORMAT_FUNCTION(1, 2);
#endif

__END_DECLS

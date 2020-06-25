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

#include "FIRCLSUserLogging.h"

#include <sys/time.h>

#include "FIRCLSGlobals.h"
#include "FIRCLSUtility.h"

#import "FIRCLSReportManager_Private.h"

NSString *const FIRCLSStartTimeKey = @"com.crashlytics.kit-start-time";
NSString *const FIRCLSFirstRunloopTurnTimeKey = @"com.crashlytics.first-run-loop-time";
NSString *const FIRCLSInBackgroundKey = @"com.crashlytics.in-background";
#if TARGET_OS_IPHONE
NSString *const FIRCLSDeviceOrientationKey = @"com.crashlytics.device-orientation";
NSString *const FIRCLSUIOrientationKey = @"com.crashlytics.ui-orientation";
#endif
NSString *const FIRCLSUserIdentifierKey = @"com.crashlytics.user-id";
NSString *const FIRCLSDevelopmentPlatformNameKey = @"com.crashlytics.development-platform-name";
NSString *const FIRCLSDevelopmentPlatformVersionKey =
    @"com.crashlytics.development-platform-version";

const uint32_t FIRCLSUserLoggingMaxKVEntries = 64;

#pragma mark - Prototypes
static void FIRCLSUserLoggingWriteKeyValue(NSString *key,
                                           NSString *value,
                                           FIRCLSUserLoggingKVStorage *storage,
                                           uint32_t *counter);
static void FIRCLSUserLoggingCheckAndSwapABFiles(FIRCLSUserLoggingABStorage *storage,
                                                 const char **activePath,
                                                 off_t fileSize);
void FIRCLSLogInternal(NSString *message);

#pragma mark - Setup
void FIRCLSUserLoggingInit(FIRCLSUserLoggingReadOnlyContext *roContext,
                           FIRCLSUserLoggingWritableContext *rwContext) {
  rwContext->activeUserLogPath = roContext->logStorage.aPath;
  rwContext->activeErrorLogPath = roContext->errorStorage.aPath;
  rwContext->activeCustomExceptionPath = roContext->customExceptionStorage.aPath;

  rwContext->userKVCount = 0;
  rwContext->internalKVCount = 0;
  rwContext->errorsCount = 0;

  roContext->userKVStorage.maxIncrementalCount = FIRCLSUserLoggingMaxKVEntries;
  roContext->internalKVStorage.maxIncrementalCount = roContext->userKVStorage.maxIncrementalCount;
}

#pragma mark - KV Logging
void FIRCLSUserLoggingRecordInternalKeyValue(NSString *key, id value) {
  FIRCLSUserLoggingRecordKeyValue(key, value, &_firclsContext.readonly->logging.internalKVStorage,
                                  &_firclsContext.writable->logging.internalKVCount);
}

void FIRCLSUserLoggingWriteInternalKeyValue(NSString *key, NSString *value) {
  // Unsynchronized - must be run on the correct queue
  FIRCLSUserLoggingWriteKeyValue(key, value, &_firclsContext.readonly->logging.internalKVStorage,
                                 &_firclsContext.writable->logging.internalKVCount);
}

void FIRCLSUserLoggingRecordUserKeyValue(NSString *key, id value) {
  FIRCLSUserLoggingRecordKeyValue(key, value, &_firclsContext.readonly->logging.userKVStorage,
                                  &_firclsContext.writable->logging.userKVCount);
}

static id FIRCLSUserLoggingGetComponent(NSDictionary *entry,
                                        NSString *componentName,
                                        bool decodeHex) {
  id value = [entry objectForKey:componentName];

  return (decodeHex && value != [NSNull null]) ? FIRCLSFileHexDecodeString([value UTF8String])
                                               : value;
}

static NSString *FIRCLSUserLoggingGetKey(NSDictionary *entry, bool decodeHex) {
  return FIRCLSUserLoggingGetComponent(entry, @"key", decodeHex);
}

static id FIRCLSUserLoggingGetValue(NSDictionary *entry, bool decodeHex) {
  return FIRCLSUserLoggingGetComponent(entry, @"value", decodeHex);
}

NSDictionary *FIRCLSUserLoggingGetCompactedKVEntries(FIRCLSUserLoggingKVStorage *storage,
                                                     bool decodeHex) {
  if (!FIRCLSIsValidPointer(storage)) {
    FIRCLSSDKLogError("storage invalid\n");
    return nil;
  }

  NSArray *incrementalKVs = FIRCLSUserLoggingStoredKeyValues(storage->incrementalPath);
  NSArray *compactedKVs = FIRCLSUserLoggingStoredKeyValues(storage->compactedPath);

  NSMutableDictionary *finalKVSet = [NSMutableDictionary new];

  // These should all be unique, so there might be a more efficient way to
  // do this
  for (NSDictionary *entry in compactedKVs) {
    NSString *key = FIRCLSUserLoggingGetKey(entry, decodeHex);
    NSString *value = FIRCLSUserLoggingGetValue(entry, decodeHex);

    if (!key || !value) {
      FIRCLSSDKLogError("compacted key/value contains a nil and must be dropped\n");
      continue;
    }

    [finalKVSet setObject:value forKey:key];
  }

  // Now, assign the incremental values, in file order, so we overwrite any older values.
  for (NSDictionary *entry in incrementalKVs) {
    NSString *key = FIRCLSUserLoggingGetKey(entry, decodeHex);
    NSString *value = FIRCLSUserLoggingGetValue(entry, decodeHex);

    if (!key || !value) {
      FIRCLSSDKLogError("incremental key/value contains a nil and must be dropped\n");
      continue;
    }

    if ([value isEqual:[NSNull null]]) {
      [finalKVSet removeObjectForKey:key];
    } else {
      [finalKVSet setObject:value forKey:key];
    }
  }

  return finalKVSet;
}

void FIRCLSUserLoggingCompactKVEntries(FIRCLSUserLoggingKVStorage *storage) {
  if (!FIRCLSIsValidPointer(storage)) {
    FIRCLSSDKLogError("Error: storage invalid\n");
    return;
  }

  NSDictionary *finalKVs = FIRCLSUserLoggingGetCompactedKVEntries(storage, false);

  if (unlink(storage->compactedPath) != 0) {
    FIRCLSSDKLog("Error: Unable to remove compacted KV store before compaction %s\n",
                 strerror(errno));
  }

  FIRCLSFile file;

  if (!FIRCLSFileInitWithPath(&file, storage->compactedPath, true)) {
    FIRCLSSDKLog("Error: Unable to open compacted k-v file\n");
    return;
  }

  uint32_t maxCount = storage->maxCount;
  if ([finalKVs count] > maxCount) {
    // We need to remove keys, to avoid going over the max.
    // This is just about the worst way to go about doing this. There are lots of smarter ways,
    // but it's very uncommon to go down this path.
    NSArray *keys = [finalKVs allKeys];

    FIRCLSSDKLogInfo("Truncating KV set, which is above max %d\n", maxCount);

    finalKVs =
        [finalKVs dictionaryWithValuesForKeys:[keys subarrayWithRange:NSMakeRange(0, maxCount)]];
  }

  for (NSString *key in finalKVs) {
    NSString *value = [finalKVs objectForKey:key];

    FIRCLSFileWriteSectionStart(&file, "kv");
    FIRCLSFileWriteHashStart(&file);
    // tricky - the values stored incrementally have already been hex-encoded
    FIRCLSFileWriteHashEntryString(&file, "key", [key UTF8String]);
    FIRCLSFileWriteHashEntryString(&file, "value", [value UTF8String]);
    FIRCLSFileWriteHashEnd(&file);
    FIRCLSFileWriteSectionEnd(&file);
  }

  FIRCLSFileClose(&file);

  if (unlink(storage->incrementalPath) != 0) {
    FIRCLSSDKLog("Error: Unable to remove incremental KV store after compaction %s\n",
                 strerror(errno));
  }
}

void FIRCLSUserLoggingRecordKeyValue(NSString *key,
                                     id value,
                                     FIRCLSUserLoggingKVStorage *storage,
                                     uint32_t *counter) {
  if (!FIRCLSIsValidPointer(key)) {
    FIRCLSSDKLogWarn("User provided bad key\n");
    return;
  }

  // ensure that any invalid pointer is actually set to nil
  if (!FIRCLSIsValidPointer(value) && value != nil) {
    FIRCLSSDKLogWarn("Bad value pointer being clamped to nil\n");
    value = nil;
  }

  if (!FIRCLSContextIsInitialized()) {
    return;
  }

  if ([value respondsToSelector:@selector(description)]) {
    value = [value description];
  } else {
    // passing nil will result in a JSON null being written, which is deserialized as [NSNull null],
    // signaling to remove the key during compaction
    value = nil;
  }

  dispatch_sync(FIRCLSGetLoggingQueue(), ^{
    FIRCLSUserLoggingWriteKeyValue(key, value, storage, counter);
  });
}

static void FIRCLSUserLoggingWriteKeyValue(NSString *key,
                                           NSString *value,
                                           FIRCLSUserLoggingKVStorage *storage,
                                           uint32_t *counter) {
  FIRCLSFile file;

  if (!FIRCLSIsValidPointer(storage) || !FIRCLSIsValidPointer(counter)) {
    FIRCLSSDKLogError("Bad parameters\n");
    return;
  }

  if (!FIRCLSFileInitWithPath(&file, storage->incrementalPath, true)) {
    FIRCLSSDKLogError("Unable to open k-v file\n");
    return;
  }

  FIRCLSFileWriteSectionStart(&file, "kv");
  FIRCLSFileWriteHashStart(&file);
  FIRCLSFileWriteHashEntryHexEncodedString(&file, "key", [key UTF8String]);
  FIRCLSFileWriteHashEntryHexEncodedString(&file, "value", [value UTF8String]);
  FIRCLSFileWriteHashEnd(&file);
  FIRCLSFileWriteSectionEnd(&file);

  FIRCLSFileClose(&file);

  *counter += 1;
  if (*counter >= storage->maxIncrementalCount) {
    dispatch_async(FIRCLSGetLoggingQueue(), ^{
      FIRCLSUserLoggingCompactKVEntries(storage);
      *counter = 0;
    });
  }
}

NSArray *FIRCLSUserLoggingStoredKeyValues(const char *path) {
  if (!FIRCLSContextIsInitialized()) {
    return nil;
  }

  return FIRCLSFileReadSections(path, true, ^NSObject *(id obj) {
    return [obj objectForKey:@"kv"];
  });
}

#pragma mark - NSError Logging
static void FIRCLSUserLoggingRecordErrorUserInfo(FIRCLSFile *file,
                                                 const char *fileKey,
                                                 NSDictionary<NSString *, id> *userInfo) {
  if ([userInfo count] == 0) {
    return;
  }

  FIRCLSFileWriteHashKey(file, fileKey);
  FIRCLSFileWriteArrayStart(file);

  for (id key in userInfo) {
    id value = [userInfo objectForKey:key];
    if (![value respondsToSelector:@selector(description)]) {
      continue;
    }

    FIRCLSFileWriteArrayStart(file);
    FIRCLSFileWriteArrayEntryHexEncodedString(file, [key UTF8String]);
    FIRCLSFileWriteArrayEntryHexEncodedString(file, [[value description] UTF8String]);
    FIRCLSFileWriteArrayEnd(file);
  }

  FIRCLSFileWriteArrayEnd(file);
}

static void FIRCLSUserLoggingWriteError(FIRCLSFile *file,
                                        NSError *error,
                                        NSDictionary<NSString *, id> *additionalUserInfo,
                                        NSArray *addresses,
                                        uint64_t timestamp) {
  FIRCLSFileWriteSectionStart(file, "error");
  FIRCLSFileWriteHashStart(file);
  FIRCLSFileWriteHashEntryHexEncodedString(file, "domain", [[error domain] UTF8String]);
  FIRCLSFileWriteHashEntryInt64(file, "code", [error code]);
  FIRCLSFileWriteHashEntryUint64(file, "time", timestamp);

  // addresses
  FIRCLSFileWriteHashKey(file, "stacktrace");
  FIRCLSFileWriteArrayStart(file);
  for (NSNumber *address in addresses) {
    FIRCLSFileWriteArrayEntryUint64(file, [address unsignedLongLongValue]);
  }
  FIRCLSFileWriteArrayEnd(file);

  // user-info
  FIRCLSUserLoggingRecordErrorUserInfo(file, "info", [error userInfo]);
  FIRCLSUserLoggingRecordErrorUserInfo(file, "extra_info", additionalUserInfo);

  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteSectionEnd(file);
}

void FIRCLSUserLoggingRecordError(NSError *error,
                                  NSDictionary<NSString *, id> *additionalUserInfo) {
  if (!error) {
    return;
  }

  if (!FIRCLSContextIsInitialized()) {
    return;
  }

  // record the stacktrace and timestamp here, so we
  // are as close as possible to the user's log statement
  NSArray *addresses = [NSThread callStackReturnAddresses];
  uint64_t timestamp = time(NULL);

  FIRCLSUserLoggingWriteAndCheckABFiles(
      &_firclsContext.readonly->logging.errorStorage,
      &_firclsContext.writable->logging.activeErrorLogPath, ^(FIRCLSFile *file) {
        FIRCLSUserLoggingWriteError(file, error, additionalUserInfo, addresses, timestamp);
      });
}

#pragma mark - CLSLog Support
void FIRCLSLog(NSString *format, ...) {
  // If the format is nil do nothing just like NSLog.
  if (!format) {
    return;
  }

  va_list args;
  va_start(args, format);
  NSString *msg = [[NSString alloc] initWithFormat:format arguments:args];
  va_end(args);

  FIRCLSLogInternal(msg);
}

#pragma mark - Properties
uint32_t FIRCLSUserLoggingMaxLogSize(void) {
  // don't forget that the message encoding overhead is 2x, and we
  // wrap everything in a json structure with time. So, there is
  // quite a penalty

  uint32_t size = 1024 * 64;

  return size * 2;
}

uint32_t FIRCLSUserLoggingMaxErrorSize(void) {
  return FIRCLSUserLoggingMaxLogSize();
}

#pragma mark - AB Logging
void FIRCLSUserLoggingCheckAndSwapABFiles(FIRCLSUserLoggingABStorage *storage,
                                          const char **activePath,
                                          off_t fileSize) {
  if (!activePath || !storage) {
    return;
  }

  if (!*activePath) {
    return;
  }

  if (storage->restrictBySize) {
    if (fileSize <= storage->maxSize) {
      return;
    }
  } else {
    if (!FIRCLSIsValidPointer(storage->entryCount)) {
      FIRCLSSDKLogError("Error: storage has invalid pointer, but is restricted by entry count\n");
      return;
    }

    if (*storage->entryCount < storage->maxEntries) {
      return;
    }

    // Here we have rolled over, so we have to reset our counter.
    *storage->entryCount = 0;
  }

  // if it is too big:
  // - reset the other log
  // - make it active
  const char *otherPath = NULL;

  if (*activePath == storage->aPath) {
    otherPath = storage->bPath;
  } else {
    // take this path if the pointer is invalid as well, to reset
    otherPath = storage->aPath;
  }

  // guard here against path being nil or empty
  NSString *pathString = [NSString stringWithUTF8String:otherPath];

  if ([pathString length] > 0) {
    // ignore the error, because there is nothing we can do to recover here, and its likely
    // any failures would be intermittent

    [[NSFileManager defaultManager] removeItemAtPath:pathString error:nil];
  }

  *activePath = otherPath;
}

void FIRCLSUserLoggingWriteAndCheckABFiles(FIRCLSUserLoggingABStorage *storage,
                                           const char **activePath,
                                           void (^openedFileBlock)(FIRCLSFile *file)) {
  if (!storage || !activePath || !openedFileBlock) {
    return;
  }

  if (!*activePath) {
    return;
  }

  if (storage->restrictBySize) {
    if (storage->maxSize == 0) {
      return;
    }
  } else {
    if (storage->maxEntries == 0) {
      return;
    }
  }

  dispatch_sync(FIRCLSGetLoggingQueue(), ^{
    FIRCLSFile file;

    if (!FIRCLSFileInitWithPath(&file, *activePath, true)) {
      FIRCLSSDKLog("Unable to open log file\n");
      return;
    }

    openedFileBlock(&file);

    off_t fileSize = 0;
    FIRCLSFileCloseWithOffset(&file, &fileSize);

    // increment the count before calling FIRCLSUserLoggingCheckAndSwapABFiles, so the value
    // reflects the actual amount of stuff written
    if (!storage->restrictBySize && FIRCLSIsValidPointer(storage->entryCount)) {
      *storage->entryCount += 1;
    }

    dispatch_async(FIRCLSGetLoggingQueue(), ^{
      FIRCLSUserLoggingCheckAndSwapABFiles(storage, activePath, fileSize);
    });
  });
}

void FIRCLSLogInternalWrite(FIRCLSFile *file, NSString *message, uint64_t time) {
  FIRCLSFileWriteSectionStart(file, "log");
  FIRCLSFileWriteHashStart(file);
  FIRCLSFileWriteHashEntryHexEncodedString(file, "msg", [message UTF8String]);
  FIRCLSFileWriteHashEntryUint64(file, "time", time);
  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteSectionEnd(file);
}

void FIRCLSLogInternal(NSString *message) {
  if (!message) {
    return;
  }

  if (!FIRCLSContextIsInitialized()) {
    FIRCLSWarningLog(@"WARNING: FIRCLSLog has been used before (or concurrently with) "
                     @"Crashlytics initialization and cannot be recorded. The message was: \n%@",
                     message);
    return;
  }
  struct timeval te;

  NSUInteger messageLength = [message length];
  int maxLogSize = _firclsContext.readonly->logging.logStorage.maxSize;

  if (messageLength > maxLogSize) {
    FIRCLSWarningLog(
        @"WARNING: Attempted to write %zd bytes, but %d is the maximum size of the log. "
        @"Truncating to %d bytes.\n",
        messageLength, maxLogSize, maxLogSize);
    message = [message substringToIndex:maxLogSize];
  }

  // unable to get time - abort
  if (gettimeofday(&te, NULL) != 0) {
    return;
  }

  const uint64_t time = te.tv_sec * 1000LL + te.tv_usec / 1000;

  FIRCLSUserLoggingWriteAndCheckABFiles(&_firclsContext.readonly->logging.logStorage,
                                        &_firclsContext.writable->logging.activeUserLogPath,
                                        ^(FIRCLSFile *file) {
                                          FIRCLSLogInternalWrite(file, message, time);
                                        });
}

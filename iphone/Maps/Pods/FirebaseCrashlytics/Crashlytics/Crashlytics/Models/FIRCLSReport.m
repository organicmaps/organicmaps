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

#import "FIRCLSContext.h"
#import "FIRCLSFile.h"
#import "FIRCLSGlobals.h"
#import "FIRCLSInternalReport.h"
#import "FIRCLSReport_Private.h"
#import "FIRCLSUserLogging.h"

@interface FIRCLSReport () {
  FIRCLSInternalReport *_internalReport;
  uint32_t _internalKVCounter;
  uint32_t _userKVCounter;

  NSString *_internalCompactedKVFile;
  NSString *_internalIncrementalKVFile;
  NSString *_userCompactedKVFile;
  NSString *_userIncrementalKVFile;

  BOOL _readOnly;

  // cached values, to ensure that their contents remain valid
  // even if the report is deleted
  NSString *_identifer;
  NSString *_bundleVersion;
  NSString *_bundleShortVersionString;
  NSDate *_dateCreated;
  NSDate *_crashedOnDate;
  NSString *_OSVersion;
  NSString *_OSBuildVersion;
  NSNumber *_isCrash;
  NSDictionary *_customKeys;
}

@end

@implementation FIRCLSReport

- (instancetype)initWithInternalReport:(FIRCLSInternalReport *)report
                          prefetchData:(BOOL)shouldPrefetch {
  self = [super init];
  if (!self) {
    return nil;
  }

  _internalReport = report;

  // TODO: correct kv accounting
  // The internal report will have non-zero compacted and incremental keys. The right thing to do
  // is count them, so we can kick off compactions/pruning at the right times. By
  // setting this value to zero, we're allowing more entries to be made than there really
  // should be. Not the end of the world, but we should do better eventually.
  _internalKVCounter = 0;
  _userKVCounter = 0;

  _internalCompactedKVFile =
      [self.internalReport pathForContentFile:FIRCLSReportInternalCompactedKVFile];
  _internalIncrementalKVFile =
      [self.internalReport pathForContentFile:FIRCLSReportInternalIncrementalKVFile];
  _userCompactedKVFile = [self.internalReport pathForContentFile:FIRCLSReportUserCompactedKVFile];
  _userIncrementalKVFile =
      [self.internalReport pathForContentFile:FIRCLSReportUserIncrementalKVFile];

  _readOnly = shouldPrefetch;

  if (shouldPrefetch) {
    _identifer = report.identifier;
    _bundleVersion = report.bundleVersion;
    _bundleShortVersionString = report.bundleShortVersionString;
    _dateCreated = report.dateCreated;
    _crashedOnDate = report.crashedOnDate;
    _OSVersion = report.OSVersion;
    _OSBuildVersion = report.OSBuildVersion;
    _isCrash = [NSNumber numberWithBool:report.isCrash];

    _customKeys = [self readCustomKeys];
  }

  return self;
}

- (instancetype)initWithInternalReport:(FIRCLSInternalReport *)report {
  return [self initWithInternalReport:report prefetchData:NO];
}

#pragma mark - Helpers
- (FIRCLSUserLoggingKVStorage)internalKVStorage {
  FIRCLSUserLoggingKVStorage storage;

  storage.maxCount = _firclsContext.readonly->logging.internalKVStorage.maxCount;
  storage.maxIncrementalCount =
      _firclsContext.readonly->logging.internalKVStorage.maxIncrementalCount;
  storage.compactedPath = [_internalCompactedKVFile fileSystemRepresentation];
  storage.incrementalPath = [_internalIncrementalKVFile fileSystemRepresentation];

  return storage;
}

- (FIRCLSUserLoggingKVStorage)userKVStorage {
  FIRCLSUserLoggingKVStorage storage;

  storage.maxCount = _firclsContext.readonly->logging.userKVStorage.maxCount;
  storage.maxIncrementalCount = _firclsContext.readonly->logging.userKVStorage.maxIncrementalCount;
  storage.compactedPath = [_userCompactedKVFile fileSystemRepresentation];
  storage.incrementalPath = [_userIncrementalKVFile fileSystemRepresentation];

  return storage;
}

- (BOOL)canRecordNewValues {
  return !_readOnly && FIRCLSContextIsInitialized();
}

- (void)recordValue:(id)value forInternalKey:(NSString *)key {
  if (!self.canRecordNewValues) {
    return;
  }

  FIRCLSUserLoggingKVStorage storage = [self internalKVStorage];

  FIRCLSUserLoggingRecordKeyValue(key, value, &storage, &_internalKVCounter);
}

- (void)recordValue:(id)value forUserKey:(NSString *)key {
  if (!self.canRecordNewValues) {
    return;
  }

  FIRCLSUserLoggingKVStorage storage = [self userKVStorage];

  FIRCLSUserLoggingRecordKeyValue(key, value, &storage, &_userKVCounter);
}

- (NSDictionary *)readCustomKeys {
  FIRCLSUserLoggingKVStorage storage = [self userKVStorage];

  // return decoded entries
  return FIRCLSUserLoggingGetCompactedKVEntries(&storage, true);
}

#pragma mark - Metadata helpers

- (NSString *)identifier {
  if (!_identifer) {
    _identifer = self.internalReport.identifier;
  }

  return _identifer;
}

- (NSDictionary *)customKeys {
  if (!_customKeys) {
    _customKeys = [self readCustomKeys];
  }

  return _customKeys;
}

- (NSString *)bundleVersion {
  if (!_bundleVersion) {
    _bundleVersion = self.internalReport.bundleVersion;
  }

  return _bundleVersion;
}

- (NSString *)bundleShortVersionString {
  if (!_bundleShortVersionString) {
    _bundleShortVersionString = self.internalReport.bundleShortVersionString;
  }

  return _bundleShortVersionString;
}

- (NSDate *)dateCreated {
  if (!_dateCreated) {
    _dateCreated = self.internalReport.dateCreated;
  }

  return _dateCreated;
}

// for compatibility with the CLSCrashReport Protocol
- (NSDate *)crashedOnDate {
  if (!_crashedOnDate) {
    _crashedOnDate = self.internalReport.crashedOnDate;
  }

  return _crashedOnDate;
}

- (NSString *)OSVersion {
  if (!_OSVersion) {
    _OSVersion = self.internalReport.OSVersion;
  }

  return _OSVersion;
}

- (NSString *)OSBuildVersion {
  if (!_OSBuildVersion) {
    _OSBuildVersion = self.internalReport.OSBuildVersion;
  }

  return _OSBuildVersion;
}

- (BOOL)isCrash {
  if (_isCrash == nil) {
    _isCrash = [NSNumber numberWithBool:self.internalReport.isCrash];
  }

  return [_isCrash boolValue];
}

#pragma mark - Public Read/Write Methods
- (void)setObjectValue:(id)value forKey:(NSString *)key {
  [self recordValue:value forUserKey:key];
}

- (NSString *)userIdentifier {
  return nil;
}

- (void)setUserIdentifier:(NSString *)userIdentifier {
  [self recordValue:userIdentifier forInternalKey:FIRCLSUserIdentifierKey];
}

@end

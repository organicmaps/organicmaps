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

#import "FIRCLSSettings.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import "FIRCLSApplicationIdentifierModel.h"
#import "FIRCLSConstants.h"
#import "FIRCLSFileManager.h"
#import "FIRCLSLogger.h"
#import "FIRCLSURLBuilder.h"

NSString *const CreatedAtKey = @"created_at";
NSString *const GoogleAppIDKey = @"google_app_id";
NSString *const BuildInstanceID = @"build_instance_id";
NSString *const AppVersion = @"app_version";

@interface FIRCLSSettings ()

@property(nonatomic, strong) FIRCLSFileManager *fileManager;
@property(nonatomic, strong) FIRCLSApplicationIdentifierModel *appIDModel;

@property(nonatomic, strong) NSDictionary<NSString *, id> *settingsDictionary;

@property(nonatomic) BOOL isCacheKeyExpired;

@end

@implementation FIRCLSSettings

- (instancetype)initWithFileManager:(FIRCLSFileManager *)fileManager
                         appIDModel:(FIRCLSApplicationIdentifierModel *)appIDModel {
  self = [super init];
  if (!self) {
    return nil;
  }

  _fileManager = fileManager;
  _appIDModel = appIDModel;

  _settingsDictionary = nil;
  _isCacheKeyExpired = NO;

  return self;
}

#pragma mark - Public Methods

- (void)reloadFromCacheWithGoogleAppID:(NSString *)googleAppID
                      currentTimestamp:(NSTimeInterval)currentTimestamp {
  NSString *settingsFilePath = self.fileManager.settingsFilePath;

  NSData *data = [self.fileManager dataWithContentsOfFile:settingsFilePath];

  if (!data) {
    FIRCLSDebugLog(@"[Crashlytics:Settings] No settings were cached");

    return;
  }

  NSError *error = nil;
  @synchronized(self) {
    _settingsDictionary = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
  }

  if (!_settingsDictionary) {
    FIRCLSErrorLog(@"Could not load settings file data with error: %@", error.localizedDescription);

    // Attempt to remove it, in case it's messed up
    [self deleteCachedSettings];
    return;
  }

  NSDictionary<NSString *, id> *cacheKey = [self loadCacheKey];
  if (!cacheKey) {
    FIRCLSErrorLog(@"Could not load settings cache key");

    [self deleteCachedSettings];
    return;
  }

  NSString *cachedGoogleAppID = cacheKey[GoogleAppIDKey];
  if (![cachedGoogleAppID isEqualToString:googleAppID]) {
    FIRCLSDebugLog(
        @"[Crashlytics:Settings] Invalidating settings cache because Google App ID changed");

    [self deleteCachedSettings];
    return;
  }

  NSTimeInterval cacheCreatedAt = [cacheKey[CreatedAtKey] unsignedIntValue];
  NSTimeInterval cacheDurationSeconds = self.cacheDurationSeconds;
  if (currentTimestamp > (cacheCreatedAt + cacheDurationSeconds)) {
    FIRCLSDebugLog(@"[Crashlytics:Settings] Settings TTL expired");

    @synchronized(self) {
      self.isCacheKeyExpired = YES;
    }
  }

  NSString *cacheBuildInstanceID = cacheKey[BuildInstanceID];
  if (![cacheBuildInstanceID isEqualToString:self.appIDModel.buildInstanceID]) {
    FIRCLSDebugLog(@"[Crashlytics:Settings] Settings expired because build instance changed");

    @synchronized(self) {
      self.isCacheKeyExpired = YES;
    }
  }

  NSString *cacheAppVersion = cacheKey[AppVersion];
  if (![cacheAppVersion isEqualToString:self.appIDModel.synthesizedVersion]) {
    FIRCLSDebugLog(@"[Crashlytics:Settings] Settings expired because app version changed");

    @synchronized(self) {
      self.isCacheKeyExpired = YES;
    }
  }
}

- (void)cacheSettingsWithGoogleAppID:(NSString *)googleAppID
                    currentTimestamp:(NSTimeInterval)currentTimestamp {
  NSNumber *createdAtTimestamp = [NSNumber numberWithDouble:currentTimestamp];
  NSDictionary *cacheKey = @{
    CreatedAtKey : createdAtTimestamp,
    GoogleAppIDKey : googleAppID,
    BuildInstanceID : self.appIDModel.buildInstanceID,
    AppVersion : self.appIDModel.synthesizedVersion,
  };

  NSError *error = nil;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:cacheKey
                                                     options:kNilOptions
                                                       error:&error];

  if (!jsonData) {
    FIRCLSErrorLog(@"Could not create settings cache key with error: %@",
                   error.localizedDescription);

    return;
  }

  if ([self.fileManager fileExistsAtPath:self.fileManager.settingsCacheKeyPath]) {
    [self.fileManager removeItemAtPath:self.fileManager.settingsCacheKeyPath];
  }
  [self.fileManager createFileAtPath:self.fileManager.settingsCacheKeyPath
                            contents:jsonData
                          attributes:nil];

  // If Settings were expired before, they should no longer be expired after this.
  // This may be set back to YES if reloading from the cache fails
  @synchronized(self) {
    self.isCacheKeyExpired = NO;
  }

  [self reloadFromCacheWithGoogleAppID:googleAppID currentTimestamp:currentTimestamp];
}

#pragma mark - Convenience Methods

- (NSDictionary *)loadCacheKey {
  NSData *cacheKeyData =
      [self.fileManager dataWithContentsOfFile:self.fileManager.settingsCacheKeyPath];

  if (!cacheKeyData) {
    return nil;
  }

  NSError *error = nil;
  NSDictionary *cacheKey = [NSJSONSerialization JSONObjectWithData:cacheKeyData
                                                           options:NSJSONReadingAllowFragments
                                                             error:&error];
  return cacheKey;
}

- (void)deleteCachedSettings {
  __weak FIRCLSSettings *weakSelf = self;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
    __strong FIRCLSSettings *strongSelf = weakSelf;
    if ([strongSelf.fileManager fileExistsAtPath:strongSelf.fileManager.settingsFilePath]) {
      [strongSelf.fileManager removeItemAtPath:strongSelf.fileManager.settingsFilePath];
    }
    if ([strongSelf.fileManager fileExistsAtPath:strongSelf.fileManager.settingsCacheKeyPath]) {
      [strongSelf.fileManager removeItemAtPath:strongSelf.fileManager.settingsCacheKeyPath];
    }
  });

  @synchronized(self) {
    self.isCacheKeyExpired = YES;
    _settingsDictionary = nil;
  }
}

- (NSDictionary<NSString *, id> *)settingsDictionary {
  @synchronized(self) {
    return _settingsDictionary;
  }
}

#pragma mark - Settings Groups

- (NSDictionary<NSString *, id> *)appSettings {
  return self.settingsDictionary[@"app"];
}

- (NSDictionary<NSString *, id> *)sessionSettings {
  return self.settingsDictionary[@"session"];
}

- (NSDictionary<NSString *, id> *)featuresSettings {
  return self.settingsDictionary[@"features"];
}

- (NSDictionary<NSString *, id> *)fabricSettings {
  return self.settingsDictionary[@"fabric"];
}

#pragma mark - Caching

- (BOOL)isCacheExpired {
  if (!self.settingsDictionary) {
    return YES;
  }

  @synchronized(self) {
    return self.isCacheKeyExpired;
  }
}

- (uint32_t)cacheDurationSeconds {
  id fetchedCacheDuration = self.settingsDictionary[@"cache_duration"];
  if (fetchedCacheDuration) {
    return [fetchedCacheDuration unsignedIntValue];
  }

  return 60 * 60;
}

#pragma mark - Identifiers

- (nullable NSString *)orgID {
  return self.fabricSettings[@"org_id"];
}

- (nullable NSString *)fetchedBundleID {
  return self.fabricSettings[@"bundle_id"];
}

#pragma mark - Onboarding / Update

- (NSString *)appStatus {
  return self.appSettings[@"status"];
}

- (BOOL)appNeedsOnboarding {
  return [self.appStatus isEqualToString:@"new"];
}

- (BOOL)appUpdateRequired {
  return [[self.appSettings objectForKey:@"update_required"] boolValue];
}

#pragma mark - On / Off Switches

- (BOOL)errorReportingEnabled {
  NSNumber *value = [self featuresSettings][@"collect_logged_exceptions"];

  if (value != nil) {
    return [value boolValue];
  }

  return YES;
}

- (BOOL)customExceptionsEnabled {
  // Right now, recording custom exceptions from the API and
  // automatically capturing non-fatal errors go hand in hand
  return [self errorReportingEnabled];
}

- (BOOL)collectReportsEnabled {
  NSNumber *value = [self featuresSettings][@"collect_reports"];

  if (value != nil) {
    return value.boolValue;
  }

  return YES;
}

- (BOOL)shouldUseNewReportEndpoint {
  NSNumber *value = [self appSettings][@"report_upload_variant"];

  // Default to use the new endpoint when settings were not successfully fetched
  // or there's an unexpected issue
  if (value == nil) {
    return YES;
  }

  // 0 - Unknown
  // 1 - Legacy
  // 2 - New
  return value.intValue == 2;
}

#pragma mark - Optional Limit Overrides

- (uint32_t)errorLogBufferSize {
  return [self logBufferSize];
}

- (uint32_t)logBufferSize {
  NSNumber *value = [self sessionSettings][@"log_buffer_size"];

  if (value != nil) {
    return value.unsignedIntValue;
  }

  return 64 * 1000;
}

- (uint32_t)maxCustomExceptions {
  NSNumber *value = [self sessionSettings][@"max_custom_exception_events"];

  if (value != nil) {
    return value.unsignedIntValue;
  }

  return 8;
}

- (uint32_t)maxCustomKeys {
  NSNumber *value = [self sessionSettings][@"max_custom_key_value_pairs"];

  if (value != nil) {
    return value.unsignedIntValue;
  }

  return 64;
}

@end

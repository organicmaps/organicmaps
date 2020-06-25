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

#import "FIRCLSDataCollectionArbiter.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import <FirebaseCore/FIRApp.h>

#import "FIRCLSUserDefaults.h"

// The legacy data collection setting allows Fabric customers to turn off auto-
// initialization, but can be overridden by calling [Fabric with:].
//
// While we support Fabric, we must have two different versions, because
// they require these slightly different semantics.
NSString *const FIRCLSLegacyCrashlyticsCollectionKey = @"firebase_crashlytics_collection_enabled";

// The new data collection setting can be set by an API that is stored in FIRCLSUserDefaults
NSString *const FIRCLSDataCollectionEnabledKey = @"com.crashlytics.data_collection";

// The new data collection setting also allows Firebase customers to turn off data
// collection in their Info.plist, and can be overridden by setting it to true using
// the setCrashlyticsCollectionEnabled API.
NSString *const FIRCLSCrashlyticsCollectionKey = @"FirebaseCrashlyticsCollectionEnabled";

typedef NS_ENUM(NSInteger, FIRCLSDataCollectionSetting) {
  FIRCLSDataCollectionSettingNotSet = 0,
  FIRCLSDataCollectionSettingEnabled = 1,
  FIRCLSDataCollectionSettingDisabled = 2,
};

@interface FIRCLSDataCollectionArbiter () {
  NSLock *_mutex;
  FBLPromise *_dataCollectionEnabled;
  BOOL _promiseResolved;
  FIRApp *_app;
  NSDictionary *_appInfo;
}
@end

@implementation FIRCLSDataCollectionArbiter

- (instancetype)initWithApp:(FIRApp *)app withAppInfo:(NSDictionary *)dict {
  self = [super init];
  if (self) {
    _mutex = [[NSLock alloc] init];
    _appInfo = dict;
    _app = app;
    if ([FIRCLSDataCollectionArbiter isCrashlyticsCollectionEnabledWithApp:app withAppInfo:dict]) {
      _dataCollectionEnabled = [FBLPromise resolvedWith:nil];
      _promiseResolved = YES;
    } else {
      _dataCollectionEnabled = [FBLPromise pendingPromise];
      _promiseResolved = NO;
    }
  }

  return self;
}

/*
 * Legacy collection key that we provide for customers to disable Crash reporting.
 * Customers can later turn on Crashlytics using Fabric.with if they choose to do so.
 *
 * This flag is unsupported for the "New SDK"
 */
- (BOOL)isLegacyDataCollectionKeyInPlist {
  if ([_appInfo objectForKey:FIRCLSLegacyCrashlyticsCollectionKey]) {
    return true;
  }

  return false;
}

// This functionality is called in the initializer before self is fully initialized,
// so a class method is used. The instance method below allows for a consistent clean API.
+ (BOOL)isCrashlyticsCollectionEnabledWithApp:(FIRApp *)app withAppInfo:(NSDictionary *)dict {
  FIRCLSDataCollectionSetting stickySetting = [FIRCLSDataCollectionArbiter stickySetting];
  if (stickySetting != FIRCLSDataCollectionSettingNotSet) {
    return stickySetting == FIRCLSDataCollectionSettingEnabled;
  }

  id firebaseCrashlyticsCollectionEnabled = [dict objectForKey:FIRCLSCrashlyticsCollectionKey];
  if ([firebaseCrashlyticsCollectionEnabled isKindOfClass:[NSString class]] ||
      [firebaseCrashlyticsCollectionEnabled isKindOfClass:[NSNumber class]]) {
    return [firebaseCrashlyticsCollectionEnabled boolValue];
  }

  return [app isDataCollectionDefaultEnabled];
}

- (BOOL)isCrashlyticsCollectionEnabled {
  return [FIRCLSDataCollectionArbiter isCrashlyticsCollectionEnabledWithApp:_app
                                                                withAppInfo:_appInfo];
}

- (void)setCrashlyticsCollectionEnabled:(BOOL)enabled {
  FIRCLSUserDefaults *userDefaults = [FIRCLSUserDefaults standardUserDefaults];
  FIRCLSDataCollectionSetting setting =
      enabled ? FIRCLSDataCollectionSettingEnabled : FIRCLSDataCollectionSettingDisabled;
  [userDefaults setInteger:setting forKey:FIRCLSDataCollectionEnabledKey];
  [userDefaults synchronize];

  [_mutex lock];
  if (enabled) {
    if (!_promiseResolved) {
      [_dataCollectionEnabled fulfill:nil];
      _promiseResolved = YES;
    }
  } else {
    if (_promiseResolved) {
      _dataCollectionEnabled = [FBLPromise pendingPromise];
      _promiseResolved = NO;
    }
  }
  [_mutex unlock];
}

+ (FIRCLSDataCollectionSetting)stickySetting {
  FIRCLSUserDefaults *userDefaults = [FIRCLSUserDefaults standardUserDefaults];
  return [userDefaults integerForKey:FIRCLSDataCollectionEnabledKey];
}

- (FBLPromise *)waitForCrashlyticsCollectionEnabled {
  FBLPromise *result = nil;
  [_mutex lock];
  result = _dataCollectionEnabled;
  [_mutex unlock];
  return result;
}

@end

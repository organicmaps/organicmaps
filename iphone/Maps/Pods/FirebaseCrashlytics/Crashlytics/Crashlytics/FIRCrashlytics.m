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

#include <stdatomic.h>

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import "FIRCLSApplicationIdentifierModel.h"
#include "FIRCLSCrashedMarkerFile.h"
#import "FIRCLSDataCollectionArbiter.h"
#import "FIRCLSDefines.h"
#include "FIRCLSException.h"
#import "FIRCLSFileManager.h"
#include "FIRCLSGlobals.h"
#import "FIRCLSHost.h"
#include "FIRCLSProfiling.h"
#import "FIRCLSReport_Private.h"
#import "FIRCLSSettings.h"
#import "FIRCLSUserDefaults.h"
#include "FIRCLSUserLogging.h"
#include "FIRCLSUtility.h"

#import "FIRCLSByteUtility.h"
#import "FIRCLSFABHost.h"
#import "FIRCLSLogger.h"

#import "FIRCLSReportManager.h"

#import <FirebaseAnalyticsInterop/FIRAnalyticsInterop.h>
#import <FirebaseCore/FIRAppInternal.h>
#import <FirebaseCore/FIRComponent.h>
#import <FirebaseCore/FIRComponentContainer.h>
#import <FirebaseCore/FIRDependency.h>
#import <FirebaseCore/FIRLibrary.h>
#import <FirebaseCore/FIROptionsInternal.h>
#import <FirebaseInstallations/FirebaseInstallations.h>

#import <GoogleDataTransport/GDTCORTargets.h>
#import <GoogleDataTransport/GDTCORTransport.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

FIRCLSContext _firclsContext;
dispatch_queue_t _firclsLoggingQueue;
dispatch_queue_t _firclsBinaryImageQueue;
dispatch_queue_t _firclsExceptionQueue;

static atomic_bool _hasInitializedInstance;

NSString *const FIRCLSGoogleTransportMappingID = @"1206";

/// Empty protocol to register with FirebaseCore's component system.
@protocol FIRCrashlyticsInstanceProvider <NSObject>
@end

@interface FIRCrashlytics () <FIRLibrary, FIRCrashlyticsInstanceProvider>

@property(nonatomic) BOOL didPreviouslyCrash;
@property(nonatomic, copy) NSString *googleAppID;
@property(nonatomic) FIRCLSDataCollectionArbiter *dataArbiter;
@property(nonatomic) FIRCLSFileManager *fileManager;
@property(nonatomic) FIRCLSReportManager *reportManager;
@property(nonatomic) GDTCORTransport *googleTransport;

@end

@implementation FIRCrashlytics

#pragma mark - Singleton Support

- (instancetype)initWithApp:(FIRApp *)app
                    appInfo:(NSDictionary *)appInfo
              installations:(FIRInstallations *)installations
                  analytics:(id<FIRAnalyticsInterop>)analytics {
  self = [super init];

  if (self) {
    bool expectedCalled = NO;
    if (!atomic_compare_exchange_strong(&_hasInitializedInstance, &expectedCalled, YES)) {
      FIRCLSErrorLog(@"Cannot instantiate more than one instance of Crashlytics.");
      return nil;
    }

    FIRCLSProfileMark mark = FIRCLSProfilingStart();

    NSLog(@"[Firebase/Crashlytics] Version %@", @CLS_SDK_DISPLAY_VERSION);

    FIRCLSDeveloperLog("Crashlytics", @"Running on %@, %@ (%@)", FIRCLSHostModelInfo(),
                       FIRCLSHostOSDisplayVersion(), FIRCLSHostOSBuildVersion());

    _googleTransport = [[GDTCORTransport alloc] initWithMappingID:FIRCLSGoogleTransportMappingID
                                                     transformers:nil
                                                           target:kGDTCORTargetCSH];

    _fileManager = [[FIRCLSFileManager alloc] init];
    _googleAppID = app.options.googleAppID;
    _dataArbiter = [[FIRCLSDataCollectionArbiter alloc] initWithApp:app withAppInfo:appInfo];

    FIRCLSApplicationIdentifierModel *appModel = [[FIRCLSApplicationIdentifierModel alloc] init];
    FIRCLSSettings *settings = [[FIRCLSSettings alloc] initWithFileManager:_fileManager
                                                                appIDModel:appModel];

    _reportManager = [[FIRCLSReportManager alloc] initWithFileManager:_fileManager
                                                        installations:installations
                                                            analytics:analytics
                                                          googleAppID:_googleAppID
                                                          dataArbiter:_dataArbiter
                                                      googleTransport:_googleTransport
                                                           appIDModel:appModel
                                                             settings:settings];

    // Process did crash during previous execution
    NSString *crashedMarkerFileName = [NSString stringWithUTF8String:FIRCLSCrashedMarkerFileName];
    NSString *crashedMarkerFileFullPath =
        [[_fileManager rootPath] stringByAppendingPathComponent:crashedMarkerFileName];
    _didPreviouslyCrash = [_fileManager fileExistsAtPath:crashedMarkerFileFullPath];

    if (_didPreviouslyCrash) {
      // Delete the crash file marker in the background ensure start up is as fast as possible
      dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [self.fileManager removeItemAtPath:crashedMarkerFileFullPath];
      });
    }

    [[[_reportManager startWithProfilingMark:mark] then:^id _Nullable(NSNumber *_Nullable value) {
      if (![value boolValue]) {
        FIRCLSErrorLog(@"Crash reporting could not be initialized");
      }
      return value;
    }] catch:^void(NSError *error) {
      FIRCLSErrorLog(@"Crash reporting failed to initialize with error: %@", error);
    }];
  }

  return self;
}

+ (void)load {
  [FIRApp registerInternalLibrary:(Class<FIRLibrary>)self
                         withName:@"firebase-crashlytics"
                      withVersion:@CLS_SDK_DISPLAY_VERSION];
}

+ (NSArray<FIRComponent *> *)componentsToRegister {
  FIRDependency *analyticsDep =
      [FIRDependency dependencyWithProtocol:@protocol(FIRAnalyticsInterop)];

  FIRComponentCreationBlock creationBlock =
      ^id _Nullable(FIRComponentContainer *container, BOOL *isCacheable) {
    if (!container.app.isDefaultApp) {
      FIRCLSErrorLog(@"Crashlytics must be used with the default Firebase app.");
      return nil;
    }

    id<FIRAnalyticsInterop> analytics = FIR_COMPONENT(FIRAnalyticsInterop, container);

    FIRInstallations *installations = [FIRInstallations installationsWithApp:container.app];

    *isCacheable = YES;

    return [[FIRCrashlytics alloc] initWithApp:container.app
                                       appInfo:NSBundle.mainBundle.infoDictionary
                                 installations:installations
                                     analytics:analytics];
  };

  FIRComponent *component =
      [FIRComponent componentWithProtocol:@protocol(FIRCrashlyticsInstanceProvider)
                      instantiationTiming:FIRInstantiationTimingEagerInDefaultApp
                             dependencies:@[ analyticsDep ]
                            creationBlock:creationBlock];
  return @[ component ];
}

+ (instancetype)crashlytics {
  // The container will return the same instance since isCacheable is set

  FIRApp *defaultApp = [FIRApp defaultApp];  // Missing configure will be logged here.

  // Get the instance from the `FIRApp`'s container. This will create a new instance the
  // first time it is called, and since `isCacheable` is set in the component creation
  // block, it will return the existing instance on subsequent calls.
  id<FIRCrashlyticsInstanceProvider> instance =
      FIR_COMPONENT(FIRCrashlyticsInstanceProvider, defaultApp.container);

  // In the component creation block, we return an instance of `FIRCrashlytics`. Cast it and
  // return it.
  return (FIRCrashlytics *)instance;
}

- (void)setCrashlyticsCollectionEnabled:(BOOL)enabled {
  [self.dataArbiter setCrashlyticsCollectionEnabled:enabled];
}

- (BOOL)isCrashlyticsCollectionEnabled {
  return [self.dataArbiter isCrashlyticsCollectionEnabled];
}

#pragma mark - API: didCrashDuringPreviousExecution

- (BOOL)didCrashDuringPreviousExecution {
  return self.didPreviouslyCrash;
}

- (void)processDidCrashDuringPreviousExecution {
  NSString *crashedMarkerFileName = [NSString stringWithUTF8String:FIRCLSCrashedMarkerFileName];
  NSString *crashedMarkerFileFullPath =
      [[self.fileManager rootPath] stringByAppendingPathComponent:crashedMarkerFileName];
  self.didPreviouslyCrash = [self.fileManager fileExistsAtPath:crashedMarkerFileFullPath];

  if (self.didPreviouslyCrash) {
    // Delete the crash file marker in the background ensure start up is as fast as possible
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
      [self.fileManager removeItemAtPath:crashedMarkerFileFullPath];
    });
  }
}

#pragma mark - API: Logging
- (void)log:(NSString *)msg {
  FIRCLSLog(@"%@", msg);
}

- (void)logWithFormat:(NSString *)format, ... {
  va_list args;
  va_start(args, format);
  [self logWithFormat:format arguments:args];
  va_end(args);
}

- (void)logWithFormat:(NSString *)format arguments:(va_list)args {
  [self log:[[NSString alloc] initWithFormat:format arguments:args]];
}

#pragma mark - API: Accessors

- (void)checkForUnsentReportsWithCompletion:(void (^)(BOOL))completion {
  [[self.reportManager checkForUnsentReports] then:^id _Nullable(NSNumber *_Nullable value) {
    completion([value boolValue]);
    return nil;
  }];
}

- (void)sendUnsentReports {
  [self.reportManager sendUnsentReports];
}

- (void)deleteUnsentReports {
  [self.reportManager deleteUnsentReports];
}

#pragma mark - API: setUserID
- (void)setUserID:(NSString *)userID {
  FIRCLSUserLoggingRecordInternalKeyValue(FIRCLSUserIdentifierKey, userID);
}

#pragma mark - API: setCustomValue

- (void)setCustomValue:(id)value forKey:(NSString *)key {
  FIRCLSUserLoggingRecordUserKeyValue(key, value);
}

#pragma mark - API: Development Platform
// These two methods are depercated by our own API, so
// its ok to implement them
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
+ (void)setDevelopmentPlatformName:(NSString *)name {
  [[self crashlytics] setDevelopmentPlatformName:name];
}

+ (void)setDevelopmentPlatformVersion:(NSString *)version {
  [[self crashlytics] setDevelopmentPlatformVersion:version];
}
#pragma clang diagnostic pop

- (NSString *)developmentPlatformName {
  FIRCLSErrorLog(@"developmentPlatformName is write-only");
  return nil;
}

- (void)setDevelopmentPlatformName:(NSString *)developmentPlatformName {
  FIRCLSUserLoggingRecordInternalKeyValue(FIRCLSDevelopmentPlatformNameKey,
                                          developmentPlatformName);
}

- (NSString *)developmentPlatformVersion {
  FIRCLSErrorLog(@"developmentPlatformVersion is write-only");
  return nil;
}

- (void)setDevelopmentPlatformVersion:(NSString *)developmentPlatformVersion {
  FIRCLSUserLoggingRecordInternalKeyValue(FIRCLSDevelopmentPlatformVersionKey,
                                          developmentPlatformVersion);
}

#pragma mark - API: Errors and Exceptions
- (void)recordError:(NSError *)error {
  FIRCLSUserLoggingRecordError(error, nil);
}

- (void)recordExceptionModel:(FIRExceptionModel *)exceptionModel {
  FIRCLSExceptionRecordModel(exceptionModel);
}

@end

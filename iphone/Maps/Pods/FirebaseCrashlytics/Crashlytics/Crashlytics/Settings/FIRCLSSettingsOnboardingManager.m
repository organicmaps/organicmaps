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

#import "FIRCLSSettingsOnboardingManager.h"

#import "FIRCLSApplicationIdentifierModel.h"
#import "FIRCLSConstants.h"
#import "FIRCLSDataCollectionToken.h"
#import "FIRCLSDefines.h"
#import "FIRCLSDownloadAndSaveSettingsOperation.h"
#import "FIRCLSFABNetworkClient.h"
#import "FIRCLSFileManager.h"
#import "FIRCLSInstallIdentifierModel.h"
#import "FIRCLSLogger.h"
#import "FIRCLSOnboardingOperation.h"
#import "FIRCLSSettings.h"
#import "FIRCLSURLBuilder.h"

@interface FIRCLSSettingsOnboardingManager () <FIRCLSDownloadAndSaveSettingsOperationDelegate,
                                               FIRCLSOnboardingOperationDelegate>

@property(nonatomic, strong) FIRCLSApplicationIdentifierModel *appIDModel;
@property(nonatomic, strong) FIRCLSInstallIdentifierModel *installIDModel;

@property(nonatomic, strong) FIRCLSSettings *settings;

@property(nonatomic, nullable, strong) FIRCLSOnboardingOperation *onboardingOperation;
@property(nonatomic, strong) FIRCLSFileManager *fileManager;

// set to YES once onboarding call has been made.
@property(nonatomic) BOOL hasAttemptedAppConfigure;

@property(nonatomic) NSDictionary *configuration;
@property(nonatomic) NSDictionary *defaultConfiguration;
@property(nonatomic, copy) NSString *googleAppID;
@property(nonatomic, copy) NSDictionary *kitVersionsByKitBundleIdentifier;
@property(nonatomic, readonly) FIRCLSFABNetworkClient *networkClient;

@end

@implementation FIRCLSSettingsOnboardingManager

- (instancetype)initWithAppIDModel:(FIRCLSApplicationIdentifierModel *)appIDModel
                    installIDModel:(FIRCLSInstallIdentifierModel *)installIDModel
                          settings:(FIRCLSSettings *)settings
                       fileManager:(FIRCLSFileManager *)fileManager
                       googleAppID:(NSString *)googleAppID {
  self = [super init];
  if (!self) {
    return nil;
  }

  _appIDModel = appIDModel;
  _installIDModel = installIDModel;
  _settings = settings;
  _fileManager = fileManager;
  _googleAppID = googleAppID;

  _networkClient = [[FIRCLSFABNetworkClient alloc] initWithQueue:nil];

  return self;
}

- (void)beginSettingsAndOnboardingWithGoogleAppId:(NSString *)googleAppID
                                            token:(FIRCLSDataCollectionToken *)token
                                waitForCompletion:(BOOL)waitForCompletion {
  NSParameterAssert(googleAppID);

  self.googleAppID = googleAppID;

  // This map helps us determine what versions of the SDK
  // are out there. We're keeping the Fabric value in there for
  // backwards compatibility
  // TODO(b/141747635)
  self.kitVersionsByKitBundleIdentifier = @{
    FIRCLSApplicationGetSDKBundleID() : @CLS_SDK_DISPLAY_VERSION,
  };

  [self beginSettingsDownload:token waitForCompletion:waitForCompletion];
}

#pragma mark Helper methods

/**
 * Makes a settings download request. If the request fails, the error is handled silently(with a log
 * statement). If the server response indicates onboarding is needed, an onboarding request is sent
 * to the server. If the onboarding request fails, the error is handled silently(with a log
 * statement).
 */
- (void)beginSettingsDownload:(FIRCLSDataCollectionToken *)token
            waitForCompletion:(BOOL)waitForCompletion {
  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

  FIRCLSDownloadAndSaveSettingsOperation *operation = nil;
  operation = [[FIRCLSDownloadAndSaveSettingsOperation alloc]
        initWithGoogleAppID:self.googleAppID
                   delegate:self
                settingsURL:self.settingsURL
      settingsDirectoryPath:self.fileManager.settingsDirectoryPath
           settingsFilePath:self.fileManager.settingsFilePath
             installIDModel:self.installIDModel
              networkClient:self.networkClient
                      token:token];

  if (waitForCompletion) {
    operation.asyncCompletion = ^(NSError *error) {
      dispatch_semaphore_signal(semaphore);
    };
  }

  [operation startWithToken:token];

  if (waitForCompletion) {
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
  }
}

- (void)beginOnboarding:(BOOL)appCreate
         endpointString:(NSString *)endpoint
                  token:(FIRCLSDataCollectionToken *)token {
  [self.onboardingOperation cancel];

  self.onboardingOperation =
      [[FIRCLSOnboardingOperation alloc] initWithDelegate:self
                                             shouldCreate:appCreate
                                              googleAppID:self.googleAppID
                         kitVersionsByKitBundleIdentifier:self.kitVersionsByKitBundleIdentifier
                                       appIdentifierModel:self.appIDModel
                                           endpointString:endpoint
                                            networkClient:self.networkClient
                                                    token:token
                                                 settings:self.settings];

  [self.onboardingOperation startWithToken:token];
}

- (void)finishNetworkingSession {
  [self.networkClient invalidateAndCancel];
}

#pragma mark FIRCLSOnboardingOperationDelegate methods

- (void)onboardingOperation:(FIRCLSOnboardingOperation *)operation
    didCompleteAppCreationWithError:(nullable NSError *)error {
  if (error) {
    FIRCLSErrorLog(@"Unable to complete application configure: %@", error);
    [self finishNetworkingSession];
    return;
  }
  self.onboardingOperation = nil;
  FIRCLSDebugLog(@"Completed configure");

  // now, go get settings, as they can change (and it completes the onboarding process)
  [self beginSettingsDownload:operation.token waitForCompletion:NO];
}

- (void)onboardingOperation:(FIRCLSOnboardingOperation *)operation
    didCompleteAppUpdateWithError:(nullable NSError *)error {
  [self finishNetworkingSession];
  if (error) {
    FIRCLSErrorLog(@"Unable to complete application update: %@", error);
    return;
  }
  self.onboardingOperation = nil;
  FIRCLSDebugLog(@"Completed application update");
}

#pragma mark FIRCLSDownloadAndSaveSettingsOperationDelegate methods

- (void)operation:(FIRCLSDownloadAndSaveSettingsOperation *)operation
    didDownloadAndSaveSettingsWithError:(nullable NSError *)error {
  if (error) {
    FIRCLSErrorLog(@"Failed to download settings %@", error);
    [self finishNetworkingSession];
    return;
  }

  FIRCLSDebugLog(@"Settings downloaded successfully");

  NSTimeInterval currentTimestamp = [NSDate timeIntervalSinceReferenceDate];
  [self.settings cacheSettingsWithGoogleAppID:self.googleAppID currentTimestamp:currentTimestamp];

  // only try this once
  if (self.hasAttemptedAppConfigure) {
    FIRCLSDebugLog(@"App already onboarded in this run of the app");
    [self finishNetworkingSession];
    return;
  }

  // Onboarding is still needed in Firebase, here are the backend app states -
  // 1. When the app is created in the Firebase console, app state: built (client settings call
  // returns app status: new)
  // 2. After onboarding call is made, app state: build_configured
  // 3. Another settings call is triggered after onboarding, app state: activated
  if ([self.settings appNeedsOnboarding]) {
    FIRCLSDebugLog(@"Starting onboarding with app create");
    self.hasAttemptedAppConfigure = YES;
    [self beginOnboarding:YES endpointString:FIRCLSConfigureEndpoint token:operation.token];
    return;
  }

  if ([self.settings appUpdateRequired]) {
    FIRCLSDebugLog(@"Starting onboarding with app update");
    self.hasAttemptedAppConfigure = YES;
    [self beginOnboarding:NO endpointString:FIRCLSConfigureEndpoint token:operation.token];
    return;
  }

  // we're all set!
  [self finishNetworkingSession];
}

- (NSURL *)settingsURL {
  // GET
  // /spi/v2/platforms/:platform/apps/:identifier/settings?build_version=1234&display_version=abc&instance=xyz&source=1
  FIRCLSURLBuilder *url = [FIRCLSURLBuilder URLWithBase:FIRCLSSettingsEndpoint];

  [url appendComponent:@"/spi/v2/platforms/"];
  [url escapeAndAppendComponent:self.appIDModel.platform];
  [url appendComponent:@"/gmp/"];
  [url escapeAndAppendComponent:self.googleAppID];
  [url appendComponent:@"/settings"];

  [url appendValue:self.appIDModel.buildVersion forQueryParam:@"build_version"];
  [url appendValue:self.appIDModel.displayVersion forQueryParam:@"display_version"];
  [url appendValue:self.appIDModel.buildInstanceID forQueryParam:@"instance"];
  [url appendValue:@(self.appIDModel.installSource) forQueryParam:@"source"];
  // TODO: find the right param name for KitVersions and add them here
  return url.URL;
}

@end

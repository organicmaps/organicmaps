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

#import <Foundation/Foundation.h>

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

@class FIRCLSApplicationIdentifierModel;
@class FIRCLSDataCollectionToken;
@class FIRCLSFileManager;
@class FIRCLSInstallIdentifierModel;
@class FIRCLSSettings;

NS_ASSUME_NONNULL_BEGIN

/**
 * Use this class to retrieve remote settings for the application from crashlytics backend, and
 * onboard the application on the server.
 */
@interface FIRCLSSettingsOnboardingManager : NSObject

/**
 * Designated Initializer.
 */
- (instancetype)initWithAppIDModel:(FIRCLSApplicationIdentifierModel *)appIDModel
                    installIDModel:(FIRCLSInstallIdentifierModel *)installIDModel
                          settings:(FIRCLSSettings *)settings
                       fileManager:(FIRCLSFileManager *)fileManager
                       googleAppID:(NSString *)googleAppID NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/**
 * This method kicks off downloading settings and onboarding for the app.
 * @param googleAppID (required) GMP id for the app.
 * @param token (required) Data collection token signifying we can make network calls
 */
- (void)beginSettingsAndOnboardingWithGoogleAppId:(NSString *)googleAppID
                                            token:(FIRCLSDataCollectionToken *)token
                                waitForCompletion:(BOOL)waitForCompletion;

@end

NS_ASSUME_NONNULL_END

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
#import "FIRCLSApplicationIdentifierModel.h"
#import "FIRCLSDataCollectionToken.h"
#import "FIRCLSNetworkOperation.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString *const FIRCLSOnboardingErrorDomain;

@class FIRCLSOnboardingOperation;
@class FIRCLSFABNetworkClient;
@class FIRCLSSettings;

/**
 * This is the protocol that a delegate of FIRCLSOnboardingOperation should follow.
 */
@protocol FIRCLSOnboardingOperationDelegate <NSObject>
@required

/**
 * This callback is for the delegate to know that app update has completed with/without an error.
 */
- (void)onboardingOperation:(FIRCLSOnboardingOperation *)operation
    didCompleteAppUpdateWithError:(nullable NSError *)error;
/**
 * This callback is for the delegate to know that app creation has completed with/without an error.
 */
- (void)onboardingOperation:(FIRCLSOnboardingOperation *)operation
    didCompleteAppCreationWithError:(nullable NSError *)error;

@end

/**
 * This class onboards the app, by making request to the backend servers.
 */
@interface FIRCLSOnboardingOperation : FIRCLSNetworkOperation

/**
 * When an error occurs during this operation, it is made available in this property.
 */
@property(nonatomic, readonly) NSError *error;

- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                              token:(FIRCLSDataCollectionToken *)token NS_UNAVAILABLE;

/**
 * Designated initializer.
 * @param delegate may be nil. Gets callbacks when app creation or updation succeeds or gets errored
 * out.
 * @param googleAppID must NOT be nil.
 * @param kitVersionsByKitBundleIdentifier may be nil. Maps Kit bundle identifier to kit version
 * being used in the app.
 * @param appIdentifierModel must NOT be nil. Used to get information required in the onboarding
 * network call.
 * @param appEndPoint must NOT be nil. Endpoint which needs to be hit with the onboarding request.
 * @param settings which are used to fetch the organization identifier.
 */
- (instancetype)initWithDelegate:(id<FIRCLSOnboardingOperationDelegate>)delegate
                        shouldCreate:(BOOL)shouldCreate
                         googleAppID:(NSString *)googleAppID
    kitVersionsByKitBundleIdentifier:(NSDictionary *)kitVersionsByKitBundleIdentifier
                  appIdentifierModel:(FIRCLSApplicationIdentifierModel *)appIdentifierModel
                      endpointString:(NSString *)appEndPoint
                       networkClient:(FIRCLSFABNetworkClient *)networkClient
                               token:(FIRCLSDataCollectionToken *)token
                            settings:(FIRCLSSettings *)settings NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

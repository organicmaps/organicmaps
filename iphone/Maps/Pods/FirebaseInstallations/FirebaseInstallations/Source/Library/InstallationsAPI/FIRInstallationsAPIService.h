/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>

@class FBLPromise<ValueType>;
@class FIRInstallationsItem;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *const kFIRInstallationsUserAgentKey;

FOUNDATION_EXPORT NSString *const kFIRInstallationsHeartbeatKey;

/**
 * The class is responsible for interacting with HTTP REST API for Installations.
 */
@interface FIRInstallationsAPIService : NSObject

/**
 * The default initializer.
 * @param APIKey The Firebase project API key (see `FIROptions.APIKey`).
 * @param projectID The Firebase project ID (see `FIROptions.projectID`).
 */
- (instancetype)initWithAPIKey:(NSString *)APIKey projectID:(NSString *)projectID;

/**
 * Sends a request to register a new FID to get auth and refresh tokens.
 * @param installation The `FIRInstallationsItem` instance with the FID to register.
 * @return A promise that is resolved with a new `FIRInstallationsItem` instance with valid tokens.
 * It is rejected with an error in case of a failure.
 */
- (FBLPromise<FIRInstallationsItem *> *)registerInstallation:(FIRInstallationsItem *)installation;

- (FBLPromise<FIRInstallationsItem *> *)refreshAuthTokenForInstallation:
    (FIRInstallationsItem *)installation;

/**
 * Sends a request to delete the installation, related auth tokens and all related data from the
 * server.
 * @param installation The installation to delete.
 * @return Returns a promise that is resolved with the passed installation on successful deletion or
 * is rejected with an error otherwise.
 */
- (FBLPromise<FIRInstallationsItem *> *)deleteInstallation:(FIRInstallationsItem *)installation;

@end

NS_ASSUME_NONNULL_END

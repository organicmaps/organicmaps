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
@class GULKeychainStorage;

NS_ASSUME_NONNULL_BEGIN

/// The user defaults suite name used to store data.
extern NSString *const kFIRInstallationsStoreUserDefaultsID;

/// The class is responsible for storing and accessing the installations data.
@interface FIRInstallationsStore : NSObject

/**
 * The default initializer.
 * @param storage The secure storage to save installations data.
 * @param accessGroup The Keychain Access Group to store and request the installations data.
 */
- (instancetype)initWithSecureStorage:(GULKeychainStorage *)storage
                          accessGroup:(nullable NSString *)accessGroup;

/**
 * Retrieves existing installation ID if there is.
 * @param appID The Firebase(Google) Application ID.
 * @param appName The Firebase Application Name.
 *
 * @return Returns a `FBLPromise` instance. The promise is resolved with a FIRInstallationsItem
 * instance if there is a valid installation stored for `appID` and `appName`. The promise is
 * rejected with a specific error when the installation has not been found or with another possible
 * error.
 */
- (FBLPromise<FIRInstallationsItem *> *)installationForAppID:(NSString *)appID
                                                     appName:(NSString *)appName;

/**
 * Saves the given installation.
 *
 * @param installationItem The installation data.
 * @return Returns a promise that is resolved with `[NSNull null]` on success.
 */
- (FBLPromise<NSNull *> *)saveInstallation:(FIRInstallationsItem *)installationItem;

/**
 * Removes installation data for the given app parameters.
 * @param appID The Firebase(Google) Application ID.
 * @param appName The Firebase Application Name.
 *
 * @return Returns a promise that is resolved with `[NSNull null]` on success.
 */
- (FBLPromise<NSNull *> *)removeInstallationForAppID:(NSString *)appID appName:(NSString *)appName;

@end

NS_ASSUME_NONNULL_END

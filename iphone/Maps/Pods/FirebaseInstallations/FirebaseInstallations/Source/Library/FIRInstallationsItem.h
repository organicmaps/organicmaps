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

#import "FIRInstallationsStatus.h"

@class FIRInstallationsStoredItem;
@class FIRInstallationsStoredAuthToken;
@class FIRInstallationsStoredIIDCheckin;

NS_ASSUME_NONNULL_BEGIN

/**
 * The class represents the required installation ID and auth token data including possible states.
 * The data is stored to Keychain via `FIRInstallationsStoredItem` which has only the storage
 * relevant data and does not contain any logic. `FIRInstallationsItem` must be used on the logic
 * level (not `FIRInstallationsStoredItem`).
 */
@interface FIRInstallationsItem : NSObject <NSCopying>

/// A `FirebaseApp` identifier.
@property(nonatomic, readonly) NSString *appID;
/// A `FirebaseApp` name.
@property(nonatomic, readonly) NSString *firebaseAppName;
///  A stable identifier that uniquely identifies the app instance.
@property(nonatomic, copy, nullable) NSString *firebaseInstallationID;
/// The `refreshToken` is used to authorize the auth token requests.
@property(nonatomic, copy, nullable) NSString *refreshToken;

@property(nonatomic, nullable) FIRInstallationsStoredAuthToken *authToken;
@property(nonatomic, assign) FIRInstallationsStatus registrationStatus;

/// Instance ID default token imported from IID store as a part of IID migration.
@property(nonatomic, nullable) NSString *IIDDefaultToken;

- (instancetype)initWithAppID:(NSString *)appID firebaseAppName:(NSString *)firebaseAppName;

/**
 * Populates `FIRInstallationsItem` properties with data from `FIRInstallationsStoredItem`.
 * @param item An instance of `FIRInstallationsStoredItem` to get data from.
 */
- (void)updateWithStoredItem:(FIRInstallationsStoredItem *)item;

/**
 * Creates a stored item with data from the object.
 * @return Returns a `FIRInstallationsStoredItem` instance with the data from the object.
 */
- (FIRInstallationsStoredItem *)storedItem;

/**
 * The installation identifier.
 * @return Returns a string uniquely identifying the installation.
 */
- (NSString *)identifier;

/**
 * The installation identifier.
 * @param appID A `FirebaseApp` identifier.
 * @param appName A `FirebaseApp` name.
 * @return Returns a string uniquely identifying the installation.
 */
+ (NSString *)identifierWithAppID:(NSString *)appID appName:(NSString *)appName;

/**
 * Generate a new Firebase Installation Identifier.
 * @return Returns a 22 characters long globally unique string created based on UUID.
 */
+ (NSString *)generateFID;

@end

NS_ASSUME_NONNULL_END

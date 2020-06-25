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

NS_ASSUME_NONNULL_BEGIN

/**
 * The enum represent possible states of the installation auth token.
 *
 * WARNING: The enum is stored to Keychain as a part of `FIRInstallationsStoredAuthToken`.
 * Modification of it can lead to incompatibility with previous version. Any modification must be
 * evaluated and, if it is really needed, the `storageVersion` must be bumped and proper migration
 * code added.
 */
typedef NS_ENUM(NSInteger, FIRInstallationsAuthTokenStatus) {
  /// An initial status or an undefined value.
  FIRInstallationsAuthTokenStatusUnknown,
  /// The auth token has been received from the server.
  FIRInstallationsAuthTokenStatusTokenReceived
};

/**
 * This class serializes and deserializes the installation data into/from `NSData` to be stored in
 * Keychain. This class is primarily used by `FIRInstallationsStore`. It is also used on the logic
 * level as a data object (see `FIRInstallationsItem.authToken`).
 *
 * WARNING: Modification of the class properties can lead to incompatibility with the stored data
 * encoded by the previous class versions. Any modification must be evaluated and, if it is really
 * needed, the `storageVersion` must be bumped and proper migration code added.
 */
@interface FIRInstallationsStoredAuthToken : NSObject <NSSecureCoding, NSCopying>
@property FIRInstallationsAuthTokenStatus status;

/// The token that can be used to authorize requests to Firebase backend.
@property(nullable, copy) NSString *token;
/// The date when the auth token expires.
@property(nullable, copy) NSDate *expirationDate;

/// The version of local storage.
@property(nonatomic, readonly) NSInteger storageVersion;

@end

NS_ASSUME_NONNULL_END

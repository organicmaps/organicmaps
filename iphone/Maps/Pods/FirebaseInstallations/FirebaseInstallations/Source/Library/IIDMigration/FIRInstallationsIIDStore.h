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

NS_ASSUME_NONNULL_BEGIN

/** The class encapsulates a port of a piece FirebaseInstanceID logic required to migrate IID. */
@interface FIRInstallationsIIDStore : NSObject

/**
 * Retrieves existing IID if present.
 * @return Returns a promise that is resolved with IID string if IID has been found or rejected with
 * an error otherwise.
 */
- (FBLPromise<NSString *> *)existingIID;

/**
 * Deletes existing IID if present.
 * @return Returns a promise that is resolved with `[NSNull null]` if the IID was successfully.
 * deleted or was not found. The promise is rejected otherwise.
 */
- (FBLPromise<NSNull *> *)deleteExistingIID;

#if TARGET_OS_OSX
/// If not `nil`, then only this keychain will be used to save and read data (see
/// `kSecMatchSearchList` and `kSecUseKeychain`. It is mostly intended to be used by unit tests.
@property(nonatomic, nullable) SecKeychainRef keychainRef;
#endif  // TARGET_OSX

@end

NS_ASSUME_NONNULL_END

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

extern NSString *const kFirebaseInstallationsErrorDomain;

typedef NS_ENUM(NSUInteger, FIRInstallationsErrorCode) {
  /** Unknown error. See `userInfo` for details. */
  FIRInstallationsErrorCodeUnknown = 0,

  /** Keychain error. See `userInfo` for details. */
  FIRInstallationsErrorCodeKeychain = 1,

  /** Server unreachable. A network error or server is unavailable. See `userInfo` for details. */
  FIRInstallationsErrorCodeServerUnreachable = 2,

  /** FirebaseApp configuration issues e.g. invalid GMP-App-ID, etc. See `userInfo` for details. */
  FIRInstallationsErrorCodeInvalidConfiguration = 3,

} NS_SWIFT_NAME(InstallationsErrorCode);

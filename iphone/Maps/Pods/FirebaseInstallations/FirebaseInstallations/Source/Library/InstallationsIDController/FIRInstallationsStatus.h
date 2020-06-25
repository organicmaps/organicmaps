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

/**
 * The enum represent possible states of the installation ID.
 *
 * WARNING: The enum is stored to Keychain as a part of `FIRInstallationsStoredItem`. Modification
 * of it can lead to incompatibility with previous version. Any modification must be evaluated and,
 * if it is really needed, the `storageVersion` must be bumped and proper migration code added.
 */
typedef NS_ENUM(NSInteger, FIRInstallationsStatus) {
  /** Represents either an initial status when a FIRInstallationsItem instance was created but not
   * stored to Keychain or an undefined status (e.g. when the status failed to deserialize).
   */
  FIRInstallationStatusUnknown,
  /// The Firebase Installation has not yet been registered with FIS.
  FIRInstallationStatusUnregistered,
  /// The Firebase Installation has successfully been registered with FIS.
  FIRInstallationStatusRegistered,
};

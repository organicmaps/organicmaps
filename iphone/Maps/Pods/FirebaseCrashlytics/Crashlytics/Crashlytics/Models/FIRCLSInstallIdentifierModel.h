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

@class FIRInstallations;

NS_ASSUME_NONNULL_BEGIN

/**
 * This class is a model for identifying an installation of an app
 */
@interface FIRCLSInstallIdentifierModel : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithInstallations:(FIRInstallations *)instanceID NS_DESIGNATED_INITIALIZER;

/**
 * Returns the backwards compatible Crashlytics Installation UUID
 */
@property(nonatomic, readonly) NSString *installID;

/**
 * To support end-users rotating Install IDs, this will check and rotate the Install ID,
 * which is a costly operation performance-wise. To keep the startup time impact down, call this in
 * a background thread.
 *
 * The block will be called on a background thread.
 */
- (void)regenerateInstallIDIfNeededWithBlock:(void (^)(BOOL didRotate))callback;

@end

NS_ASSUME_NONNULL_END

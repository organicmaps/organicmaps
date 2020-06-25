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

#import "FIRCLSApplication.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * This class is a model for identifiers related to the application binary.
 * It is thread-safe.
 */
@interface FIRCLSApplicationIdentifierModel : NSObject

@property(nonatomic, readonly, nullable) NSString* bundleID;

/**
 * Returns the user-facing app name
 */
@property(nonatomic, readonly, nullable) NSString* displayName;

@property(nonatomic, readonly, nullable) NSString* platform;
@property(nonatomic, readonly, nullable) NSString* buildVersion;
@property(nonatomic, readonly, nullable) NSString* displayVersion;

/**
 * Returns the synthesized app version, similar to how the backend does it
 * <displayVersion> (<buildVersion>)
 */
@property(nonatomic, readonly, nullable) NSString* synthesizedVersion;

@property(nonatomic, readonly) FIRCLSApplicationInstallationSourceType installSource;

/**
 * A mapping between all supported architectures and their UUIDs
 */
@property(nonatomic, readonly) NSDictionary* architectureUUIDMap;

/**
 * Returns the linked OS SDK
 */
@property(nonatomic, readonly) NSString* builtSDKString;

/**
 * Returns the min supported OS
 */
@property(nonatomic, readonly) NSString* minimumSDKString;

/**
 * The unique identifier for this instance of the version of app running Crashlytics. This is
 * computed by hashing the app itself.
 *
 * On Android, this is called the Build ID
 */
@property(nonatomic, readonly) NSString* buildInstanceID;

@end

NS_ASSUME_NONNULL_END

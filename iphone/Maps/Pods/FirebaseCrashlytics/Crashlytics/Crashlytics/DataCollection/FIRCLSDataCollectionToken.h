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

NS_ASSUME_NONNULL_BEGIN

/**
 * A FIRCLSDataCollectionToken represents having permission to upload data. A data collection token
 * is either valid or nil. Every function that directly initiates a network operation that will
 * result in data collection must check to make sure it has been passed a valid token. Tokens should
 * only be created when either (1) automatic data collection is enabled, or (2) the user has
 * explicitly given permission to collect data for a particular purpose, using the API. For all the
 * functions in between, the data collection token getting passed as an argument helps to document
 * and enforce the flow of data collection permission through the SDK.
 */
@interface FIRCLSDataCollectionToken : NSObject

/**
 * Creates a valid token. Only call this method when either (1) automatic data collection is
 * enabled, or (2) the user has explicitly given permission to collect data for a particular
 * purpose, using the API.
 */
+ (instancetype)validToken;

/**
 * Use this to verify that a token is valid. If this is called on a nil instance, it will return NO.
 * @return YES.
 */
- (BOOL)isValid;

@end

NS_ASSUME_NONNULL_END

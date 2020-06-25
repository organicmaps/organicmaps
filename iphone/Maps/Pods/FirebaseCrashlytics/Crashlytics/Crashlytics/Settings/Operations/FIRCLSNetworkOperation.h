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
#import "FIRCLSOperation.h"

NS_ASSUME_NONNULL_BEGIN

@class FIRCLSDataCollectionToken;
@class FIRCLSSettings;

/**
 * This is a base class for network based operations.
 */
@interface FIRCLSNetworkOperation : FIRCLSFABAsyncOperation

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/**
 * Designated initializer. All parameters are mandatory and must not be nil.
 */
- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                              token:(FIRCLSDataCollectionToken *)token NS_DESIGNATED_INITIALIZER;

- (void)start NS_UNAVAILABLE;
- (void)startWithToken:(FIRCLSDataCollectionToken *)token;

/**
 * Creates a mutable request for posting to Crashlytics backend with a default timeout.
 */
- (NSMutableURLRequest *)mutableRequestWithDefaultHTTPHeaderFieldsAndTimeoutForURL:(NSURL *)url;

/**
 * Creates a mutable request for posting to Crashlytics backend with given timeout.
 */
- (NSMutableURLRequest *)mutableRequestWithDefaultHTTPHeadersForURL:(NSURL *)url
                                                            timeout:(NSTimeInterval)timeout;

@property(nonatomic, strong, readonly) FIRCLSDataCollectionToken *token;

@end

NS_ASSUME_NONNULL_END

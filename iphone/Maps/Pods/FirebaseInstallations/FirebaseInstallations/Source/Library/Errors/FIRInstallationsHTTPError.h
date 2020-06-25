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

/** Represents an error caused by an unexpected API response. */
@interface FIRInstallationsHTTPError : NSError

@property(nonatomic, readonly) NSHTTPURLResponse *HTTPResponse;
@property(nonatomic, readonly, nonnull) NSData *data;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithHTTPResponse:(NSHTTPURLResponse *)HTTPResponse data:(nullable NSData *)data;

@end

NS_ASSUME_NONNULL_END

typedef NS_ENUM(NSInteger, FIRInstallationsHTTPCodes) {
  FIRInstallationsHTTPCodesTooManyRequests = 429,
  FIRInstallationsHTTPCodesServerInternalError = 500,
};

/** Possible response HTTP codes for `CreateInstallation` API request. */
typedef NS_ENUM(NSInteger, FIRInstallationsRegistrationHTTPCode) {
  FIRInstallationsRegistrationHTTPCodeSuccess = 201,
  FIRInstallationsRegistrationHTTPCodeInvalidArgument = 400,
  FIRInstallationsRegistrationHTTPCodeInvalidAPIKey = 401,
  FIRInstallationsRegistrationHTTPCodeAPIKeyToProjectIDMismatch = 403,
  FIRInstallationsRegistrationHTTPCodeProjectNotFound = 404,
  FIRInstallationsRegistrationHTTPCodeTooManyRequests = 429,
  FIRInstallationsRegistrationHTTPCodeServerInternalError = 500
};

typedef NS_ENUM(NSInteger, FIRInstallationsAuthTokenHTTPCode) {
  FIRInstallationsAuthTokenHTTPCodeInvalidAuthentication = 401,
  FIRInstallationsAuthTokenHTTPCodeFIDNotFound = 404,
};

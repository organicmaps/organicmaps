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

/**
 * Type to indicate response status
 */
typedef NS_ENUM(NSInteger, FIRCLSNetworkClientResponseType) {
  FIRCLSNetworkClientResponseSuccess,
  FIRCLSNetworkClientResponseInvalid,
  FIRCLSNetworkClientResponseFailure,
  FIRCLSNetworkClientResponseRetry,
  FIRCLSNetworkClientResponseBackOff
};

typedef NS_ENUM(NSInteger, FIRCLSNetworkErrorType) {
  FIRCLSNetworkErrorUnknown = -1,
  FIRCLSNetworkErrorFailedToStartOperation = -3,
  FIRCLSNetworkErrorResponseInvalid = -4,
  FIRCLSNetworkErrorRequestFailed = -5,
  FIRCLSNetworkErrorMaximumAttemptsReached = -6,
};

extern NSInteger const FIRCLSNetworkErrorUnknownURLCancelReason;

/**
 * This block is an input parameter to handleCompletedResponse: and handleCompletedTask: methods of
 * this class.
 * @param retryMightSucceed is YES if the request should be retried.
 * @param error is the error received back in response.
 */
typedef void (^FIRCLSNetworkResponseCompletionHandlerBlock)(BOOL retryMightSucceed, NSError *error);

/**
 * Error domain for Crashlytics network errors
 */
extern NSString *const FIRCLSNetworkErrorDomain;
/**
 * This class handles network responses.
 */
@interface FIRCLSNetworkResponseHandler : NSObject
/**
 * Returns the header in the given NSURLResponse with name as key
 */
+ (NSString *)headerForResponse:(NSURLResponse *)response withKey:(NSString *)key;
/**
 * Returns Retry-After header value in response, and if absent returns a default retry value
 */
+ (NSTimeInterval)retryValueForResponse:(NSURLResponse *)response;
/**
 * Checks if the content type for response matches the request
 */
+ (BOOL)contentTypeForResponse:(NSURLResponse *)response matchesRequest:(NSURLRequest *)request;

+ (NSInteger)cancelReasonFromURLError:(NSError *)error;

+ (BOOL)retryableURLError:(NSError *)error;

/**
 * Convenience method that calls back the input block with FIRCLSNetworkClientResponseType after
 * checking the response code in response
 */
+ (void)clientResponseType:(NSURLResponse *)response
                   handler:(void (^)(FIRCLSNetworkClientResponseType type,
                                     NSInteger statusCode))responseTypeAndStatusCodeHandlerBlock;
/**
 * Handles a completed response for request and calls back input block. Populates error even if
 * error was nil, but response code indicated an error.
 */
+ (void)handleCompletedResponse:(NSURLResponse *)response
             forOriginalRequest:(NSURLRequest *)originalRequest
                          error:(NSError *)error
                          block:(FIRCLSNetworkResponseCompletionHandlerBlock)completionHandlerBlock;

@end

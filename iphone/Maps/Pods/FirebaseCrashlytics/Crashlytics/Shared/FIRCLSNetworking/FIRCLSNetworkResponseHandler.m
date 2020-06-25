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

#import "FIRCLSNetworkResponseHandler.h"

@implementation FIRCLSNetworkResponseHandler

static const NSTimeInterval kFIRCLSNetworkResponseHandlerDefaultRetryInterval = 2.0;
static NSString *const kFIRCLSNetworkResponseHandlerContentType = @"Content-Type";
NSString *const FIRCLSNetworkErrorDomain = @"FIRCLSNetworkError";

NSInteger const FIRCLSNetworkErrorUnknownURLCancelReason = -1;

#pragma mark - Header Handling
+ (NSString *)headerForResponse:(NSURLResponse *)response withKey:(NSString *)key {
  if (![response respondsToSelector:@selector(allHeaderFields)]) {
    return nil;
  }

  return [((NSHTTPURLResponse *)response).allHeaderFields objectForKey:key];
}

+ (NSTimeInterval)retryValueForResponse:(NSURLResponse *)response {
  NSString *retryValueString = [self headerForResponse:response withKey:@"Retry-After"];
  if (!retryValueString) {
    return kFIRCLSNetworkResponseHandlerDefaultRetryInterval;
  }

  NSTimeInterval value = retryValueString.doubleValue;
  if (value < 0.0) {
    return kFIRCLSNetworkResponseHandlerDefaultRetryInterval;
  }

  return value;
}

+ (NSString *)requestIdForResponse:(NSURLResponse *)response {
  return [self headerForResponse:response withKey:@"X-Request-Id"];
}

+ (BOOL)contentTypeForResponse:(NSURLResponse *)response matchesRequest:(NSURLRequest *)request {
  NSString *accept = [request.allHTTPHeaderFields objectForKey:@"Accept"];
  if (!accept) {
    // An omitted accept header is defined to match everything
    return YES;
  }

  NSString *contentHeader = [self.class headerForResponse:response
                                                  withKey:kFIRCLSNetworkResponseHandlerContentType];
  if (!contentHeader) {
    //        FIRCLSDeveloperLog("Network", @"Content-Type not present in response");
    return NO;
  }

  NSString *acceptCharset = request.allHTTPHeaderFields[@"Accept-Charset"];

  NSArray *parts = [contentHeader componentsSeparatedByString:@"; charset="];
  if (!parts) {
    parts = @[ contentHeader ];
  }

  if ([[parts objectAtIndex:0] caseInsensitiveCompare:accept] != NSOrderedSame) {
    //        FIRCLSDeveloperLog("Network", @"Content-Type does not match Accept");
    return NO;
  }

  if (!acceptCharset) {
    return YES;
  }

  if (parts.count < 2) {
    return YES;
  }

  return [[parts objectAtIndex:1] caseInsensitiveCompare:acceptCharset] == NSOrderedSame;
}

+ (NSInteger)cancelReasonFromURLError:(NSError *)error {
  if (![[error domain] isEqualToString:NSURLErrorDomain]) {
    return FIRCLSNetworkErrorUnknownURLCancelReason;
  }

  if ([error code] != NSURLErrorCancelled) {
    return FIRCLSNetworkErrorUnknownURLCancelReason;
  }

  NSNumber *reason = [[error userInfo] objectForKey:NSURLErrorBackgroundTaskCancelledReasonKey];
  if (reason == nil) {
    return FIRCLSNetworkErrorUnknownURLCancelReason;
  }

  return [reason integerValue];
}

+ (BOOL)retryableURLError:(NSError *)error {
  // So far, the only task errors seen are NSURLErrorDomain. For others, we're not
  // sure what to do.
  if (![[error domain] isEqualToString:NSURLErrorDomain]) {
    return NO;
  }

  // cases that we know are definitely not retryable
  switch ([error code]) {
    case NSURLErrorBadURL:
    case NSURLErrorUnsupportedURL:
    case NSURLErrorHTTPTooManyRedirects:
    case NSURLErrorRedirectToNonExistentLocation:
    case NSURLErrorUserCancelledAuthentication:
    case NSURLErrorUserAuthenticationRequired:
    case NSURLErrorAppTransportSecurityRequiresSecureConnection:
    case NSURLErrorFileDoesNotExist:
    case NSURLErrorFileIsDirectory:
    case NSURLErrorDataLengthExceedsMaximum:
    case NSURLErrorSecureConnectionFailed:
    case NSURLErrorServerCertificateHasBadDate:
    case NSURLErrorServerCertificateUntrusted:
    case NSURLErrorServerCertificateHasUnknownRoot:
    case NSURLErrorServerCertificateNotYetValid:
    case NSURLErrorClientCertificateRejected:
    case NSURLErrorClientCertificateRequired:
    case NSURLErrorBackgroundSessionRequiresSharedContainer:
      return NO;
  }

  // All other errors, as far as I can tell, are things that could clear up
  // without action on the part of the client.

  // NSURLErrorCancelled is a potential special-case. I believe there are
  // situations where a cancelled request cannot be successfully restarted. But,
  // until I can prove it, we'll retry. There are defnitely many cases where
  // a cancelled request definitely can be restarted and will work.

  return YES;
}

#pragma mark - Error Creation
+ (NSError *)errorForCode:(NSInteger)code userInfo:(NSDictionary *)userInfo {
  return [NSError errorWithDomain:FIRCLSNetworkErrorDomain code:code userInfo:userInfo];
}

+ (NSError *)errorForResponse:(NSURLResponse *)response
                       ofType:(FIRCLSNetworkClientResponseType)type
                       status:(NSInteger)status {
  if (type == FIRCLSNetworkClientResponseSuccess) {
    return nil;
  }

  NSString *requestId = [self requestIdForResponse:response];
  NSString *contentType = [self headerForResponse:response
                                          withKey:kFIRCLSNetworkResponseHandlerContentType];

  // this could be nil, so be careful
  requestId = requestId ? requestId : @"";
  contentType = contentType ? contentType : @"";

  NSDictionary *userInfo = @{
    @"type" : @(type),
    @"status_code" : @(status),
    @"request_id" : requestId,
    @"content_type" : contentType
  };

  // compute a reasonable error code type
  NSInteger errorCode = FIRCLSNetworkErrorUnknown;
  switch (type) {
    case FIRCLSNetworkClientResponseFailure:
      errorCode = FIRCLSNetworkErrorRequestFailed;
      break;
    case FIRCLSNetworkClientResponseInvalid:
      errorCode = FIRCLSNetworkErrorResponseInvalid;
      break;
    default:
      break;
  }

  return [self errorForCode:errorCode userInfo:userInfo];
}

+ (void)clientResponseType:(NSURLResponse *)response
                   handler:(void (^)(FIRCLSNetworkClientResponseType type,
                                     NSInteger statusCode))responseTypeAndStatusCodeHandlerBlock {
  if (![response respondsToSelector:@selector(statusCode)]) {
    responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseInvalid, 0);
    return;
  }

  NSInteger code = ((NSHTTPURLResponse *)response).statusCode;

  switch (code) {
    case 200:
    case 201:
    case 202:
    case 204:
    case 304:
      responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseSuccess, code);
      return;
    case 420:
    case 429:
      responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseBackOff, code);
      return;
    case 408:
      responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseRetry, code);
      return;
    case 400:
    case 401:
    case 403:
    case 404:
    case 406:
    case 410:
    case 411:
    case 413:
    case 419:
    case 422:
    case 431:
      responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseFailure, code);
      return;
  }

  // check for a 5xx
  if (code >= 500 && code <= 599) {
    responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseRetry, code);
    return;
  }

  responseTypeAndStatusCodeHandlerBlock(FIRCLSNetworkClientResponseInvalid, code);
}

+ (void)handleCompletedResponse:(NSURLResponse *)response
             forOriginalRequest:(NSURLRequest *)originalRequest
                          error:(NSError *)originalError
                          block:
                              (FIRCLSNetworkResponseCompletionHandlerBlock)completionHandlerBlock {
  // if we have an error, we can just continue
  if (originalError) {
    BOOL retryable = [self retryableURLError:originalError];

    completionHandlerBlock(retryable, originalError);
    return;
  }

  [self.class clientResponseType:response
                         handler:^(FIRCLSNetworkClientResponseType type, NSInteger statusCode) {
                           NSError *error = nil;

                           switch (type) {
                             case FIRCLSNetworkClientResponseInvalid:
                               error = [self errorForResponse:response
                                                       ofType:type
                                                       status:statusCode];
                               break;
                             case FIRCLSNetworkClientResponseBackOff:
                             case FIRCLSNetworkClientResponseRetry:
                               error = [self errorForResponse:response
                                                       ofType:type
                                                       status:statusCode];
                               completionHandlerBlock(YES, error);
                               return;
                             case FIRCLSNetworkClientResponseFailure:
                               error = [self errorForResponse:response
                                                       ofType:type
                                                       status:statusCode];
                               break;
                             case FIRCLSNetworkClientResponseSuccess:
                               if (![self contentTypeForResponse:response
                                                  matchesRequest:originalRequest]) {
                                 error = [self errorForResponse:response
                                                         ofType:FIRCLSNetworkClientResponseInvalid
                                                         status:statusCode];
                                 break;
                               }

                               break;
                           }

                           completionHandlerBlock(NO, error);
                         }];
}

@end

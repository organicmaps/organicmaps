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

#import "FIRInstallationsErrorUtil.h"

#import "FIRInstallationsHTTPError.h"

NSString *const kFirebaseInstallationsErrorDomain = @"com.firebase.installations";

void FIRInstallationsItemSetErrorToPointer(NSError *error, NSError **pointer) {
  if (pointer != NULL) {
    *pointer = error;
  }
}

@implementation FIRInstallationsErrorUtil

+ (NSError *)keyedArchiverErrorWithException:(NSException *)exception {
  NSString *failureReason = [NSString
      stringWithFormat:@"NSKeyedArchiver exception with name: %@, reason: %@, userInfo: %@",
                       exception.name, exception.reason, exception.userInfo];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (NSError *)keyedArchiverErrorWithError:(NSError *)error {
  NSString *failureReason = [NSString stringWithFormat:@"NSKeyedArchiver error."];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:error];
}

+ (NSError *)keychainErrorWithFunction:(NSString *)keychainFunction status:(OSStatus)status {
  NSString *failureReason = [NSString stringWithFormat:@"%@ (%li)", keychainFunction, (long)status];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeKeychain
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (NSError *)installationItemNotFoundForAppID:(NSString *)appID appName:(NSString *)appName {
  NSString *failureReason =
      [NSString stringWithFormat:@"Installation for appID %@ appName %@ not found", appID, appName];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (NSError *)corruptedIIDTokenData {
  NSString *failureReason =
      @"IID token data stored in Keychain is corrupted or in an incompatible format.";
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (FIRInstallationsHTTPError *)APIErrorWithHTTPResponse:(NSHTTPURLResponse *)HTTPResponse
                                                   data:(nullable NSData *)data {
  return [[FIRInstallationsHTTPError alloc] initWithHTTPResponse:HTTPResponse data:data];
}

+ (BOOL)isAPIError:(NSError *)error withHTTPCode:(NSInteger)HTTPCode {
  if (![error isKindOfClass:[FIRInstallationsHTTPError class]]) {
    return NO;
  }

  return [(FIRInstallationsHTTPError *)error HTTPResponse].statusCode == HTTPCode;
}

+ (NSError *)JSONSerializationError:(NSError *)error {
  NSString *failureReason = [NSString stringWithFormat:@"Failed to serialize JSON data."];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (NSError *)FIDRegistrationErrorWithResponseMissingField:(NSString *)missingFieldName {
  NSString *failureReason = [NSString
      stringWithFormat:@"A required response field with name %@ is missing", missingFieldName];
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:failureReason
                          underlyingError:nil];
}

+ (NSError *)networkErrorWithError:(NSError *)error {
  return [self installationsErrorWithCode:FIRInstallationsErrorCodeServerUnreachable
                            failureReason:@"Network connection error."
                          underlyingError:error];
}

+ (NSError *)publicDomainErrorWithError:(NSError *)error {
  if ([error.domain isEqualToString:kFirebaseInstallationsErrorDomain]) {
    return error;
  }

  return [self installationsErrorWithCode:FIRInstallationsErrorCodeUnknown
                            failureReason:nil
                          underlyingError:error];
}

+ (NSError *)installationsErrorWithCode:(FIRInstallationsErrorCode)code
                          failureReason:(nullable NSString *)failureReason
                        underlyingError:(nullable NSError *)underlyingError {
  NSMutableDictionary *userInfo = [NSMutableDictionary dictionary];
  userInfo[NSUnderlyingErrorKey] = underlyingError;
  userInfo[NSLocalizedFailureReasonErrorKey] = failureReason;

  return [NSError errorWithDomain:kFirebaseInstallationsErrorDomain code:code userInfo:userInfo];
}

@end

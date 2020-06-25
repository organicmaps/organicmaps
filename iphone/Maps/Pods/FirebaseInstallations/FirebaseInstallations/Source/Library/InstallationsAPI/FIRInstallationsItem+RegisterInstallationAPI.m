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

#import "FIRInstallationsItem+RegisterInstallationAPI.h"

#import "FIRInstallationsErrorUtil.h"
#import "FIRInstallationsStoredAuthToken.h"

@implementation FIRInstallationsItem (RegisterInstallationAPI)

- (nullable FIRInstallationsItem *)
    registeredInstallationWithJSONData:(NSData *)data
                                  date:(NSDate *)date
                                 error:(NSError *__autoreleasing _Nullable *_Nullable)outError {
  NSDictionary *responseJSON = [FIRInstallationsItem dictionaryFromJSONData:data error:outError];
  if (!responseJSON) {
    return nil;
  }

  NSString *refreshToken = [FIRInstallationsItem validStringOrNilForKey:@"refreshToken"
                                                               fromDict:responseJSON];
  if (refreshToken == nil) {
    FIRInstallationsItemSetErrorToPointer(
        [FIRInstallationsErrorUtil FIDRegistrationErrorWithResponseMissingField:@"refreshToken"],
        outError);
    return nil;
  }

  NSDictionary *authTokenDict = responseJSON[@"authToken"];
  if (![authTokenDict isKindOfClass:[NSDictionary class]]) {
    FIRInstallationsItemSetErrorToPointer(
        [FIRInstallationsErrorUtil FIDRegistrationErrorWithResponseMissingField:@"authToken"],
        outError);
    return nil;
  }

  FIRInstallationsStoredAuthToken *authToken =
      [FIRInstallationsItem authTokenWithJSONDict:authTokenDict date:date error:outError];
  if (authToken == nil) {
    return nil;
  }

  FIRInstallationsItem *installation =
      [[FIRInstallationsItem alloc] initWithAppID:self.appID firebaseAppName:self.firebaseAppName];
  NSString *installationID = [FIRInstallationsItem validStringOrNilForKey:@"fid"
                                                                 fromDict:responseJSON];
  installation.firebaseInstallationID = installationID ?: self.firebaseInstallationID;
  installation.refreshToken = refreshToken;
  installation.authToken = authToken;
  installation.registrationStatus = FIRInstallationStatusRegistered;

  return installation;
}

#pragma mark - Auth token

+ (nullable FIRInstallationsStoredAuthToken *)authTokenWithGenerateTokenAPIJSONData:(NSData *)data
                                                                               date:(NSDate *)date
                                                                              error:(NSError **)
                                                                                        outError {
  NSDictionary *dict = [self dictionaryFromJSONData:data error:outError];
  if (!dict) {
    return nil;
  }

  return [self authTokenWithJSONDict:dict date:date error:outError];
}

+ (nullable FIRInstallationsStoredAuthToken *)authTokenWithJSONDict:
                                                  (NSDictionary<NSString *, id> *)dict
                                                               date:(NSDate *)date
                                                              error:(NSError **)outError {
  NSString *token = [self validStringOrNilForKey:@"token" fromDict:dict];
  if (token == nil) {
    FIRInstallationsItemSetErrorToPointer(
        [FIRInstallationsErrorUtil FIDRegistrationErrorWithResponseMissingField:@"authToken.token"],
        outError);
    return nil;
  }

  NSString *expiresInString = [self validStringOrNilForKey:@"expiresIn" fromDict:dict];
  if (expiresInString == nil) {
    FIRInstallationsItemSetErrorToPointer(
        [FIRInstallationsErrorUtil
            FIDRegistrationErrorWithResponseMissingField:@"authToken.expiresIn"],
        outError);
    return nil;
  }

  // The response should contain the string in format like "604800s".
  // The server should never response with anything else except seconds.
  // Just drop the last character and parse a number from string.
  NSString *expiresInSeconds = [expiresInString substringToIndex:expiresInString.length - 1];
  NSTimeInterval expiresIn = [expiresInSeconds doubleValue];
  NSDate *expirationDate = [date dateByAddingTimeInterval:expiresIn];

  FIRInstallationsStoredAuthToken *authToken = [[FIRInstallationsStoredAuthToken alloc] init];
  authToken.status = FIRInstallationsAuthTokenStatusTokenReceived;
  authToken.token = token;
  authToken.expirationDate = expirationDate;

  return authToken;
}

#pragma mark - JSON

+ (nullable NSDictionary<NSString *, id> *)dictionaryFromJSONData:(NSData *)data
                                                            error:(NSError **)outError {
  NSError *error;
  NSDictionary *responseJSON = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];

  if (![responseJSON isKindOfClass:[NSDictionary class]]) {
    FIRInstallationsItemSetErrorToPointer([FIRInstallationsErrorUtil JSONSerializationError:error],
                                          outError);
    return nil;
  }

  return responseJSON;
}

+ (NSString *)validStringOrNilForKey:(NSString *)key fromDict:(NSDictionary *)dict {
  NSString *string = dict[key];
  if ([string isKindOfClass:[NSString class]] && string.length > 0) {
    return string;
  }
  return nil;
}

@end

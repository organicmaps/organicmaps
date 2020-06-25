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

#import "FIRInstallationsItem.h"

@class FIRInstallationsStoredAuthToken;

NS_ASSUME_NONNULL_BEGIN

@interface FIRInstallationsItem (RegisterInstallationAPI)

/**
 * Parses and validates the Register Installation API response and returns a corresponding
 * `FIRInstallationsItem` instance on success.
 * @param JSONData The data with JSON encoded API response.
 * @param date The Auth Token expiration date will be calculated as `date` +
 * `response.authToken.expiresIn`. For most of the cases `[NSDate date]` should be passed there. A
 * different value may be passed e.g. for unit tests.
 * @param outError A pointer to assign a specific `NSError` instance in case of failure. No error is
 * assigned in case of success.
 * @return Returns a new `FIRInstallationsItem` instance in the success case or `nil` otherwise.
 */
- (nullable FIRInstallationsItem *)registeredInstallationWithJSONData:(NSData *)JSONData
                                                                 date:(NSDate *)date
                                                                error:
                                                                    (NSError *_Nullable *)outError;

+ (nullable FIRInstallationsStoredAuthToken *)authTokenWithGenerateTokenAPIJSONData:(NSData *)data
                                                                               date:(NSDate *)date
                                                                              error:(NSError **)
                                                                                        outError;

+ (nullable FIRInstallationsStoredAuthToken *)authTokenWithJSONDict:
                                                  (NSDictionary<NSString *, id> *)dict
                                                               date:(NSDate *)date
                                                              error:(NSError **)outError;

@end

NS_ASSUME_NONNULL_END

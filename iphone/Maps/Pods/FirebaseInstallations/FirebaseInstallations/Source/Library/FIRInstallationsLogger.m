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

#import "FIRInstallationsLogger.h"

FIRLoggerService kFIRLoggerInstallations = @"[Firebase/Installations]";

// FIRInstallationsAPIService.m
NSString *const kFIRInstallationsMessageCodeSendAPIRequest = @"I-FIS001001";
NSString *const kFIRInstallationsMessageCodeAPIRequestNetworkError = @"I-FIS001002";
NSString *const kFIRInstallationsMessageCodeAPIRequestResponse = @"I-FIS001003";
NSString *const kFIRInstallationsMessageCodeUnexpectedAPIRequestResponse = @"I-FIS001004";
NSString *const kFIRInstallationsMessageCodeParsingAPIResponse = @"I-FIS001005";
NSString *const kFIRInstallationsMessageCodeAPIResponseParsingInstallationFailed = @"I-FIS001006";
NSString *const kFIRInstallationsMessageCodeAPIResponseParsingInstallationSucceed = @"I-FIS001007";
NSString *const kFIRInstallationsMessageCodeAPIResponseParsingAuthTokenFailed = @"I-FIS001008";
NSString *const kFIRInstallationsMessageCodeAPIResponseParsingAuthTokenSucceed = @"I-FIS001009";

// FIRInstallationsIDController.m
NSString *const kFIRInstallationsMessageCodeNewGetInstallationOperationCreated = @"I-FIS002000";
NSString *const kFIRInstallationsMessageCodeNewGetAuthTokenOperationCreated = @"I-FIS002001";
NSString *const kFIRInstallationsMessageCodeNewDeleteInstallationOperationCreated = @"I-FIS002002";
NSString *const kFIRInstallationsMessageCodeInvalidFirebaseConfiguration = @"I-FIS002003";

// FIRInstallationsStoredItem.m
NSString *const kFIRInstallationsMessageCodeInstallationCoderVersionMismatch = @"I-FIS003000";

// FIRInstallationsStoredAuthToken.m
NSString *const kFIRInstallationsMessageCodeAuthTokenCoderVersionMismatch = @"I-FIS004000";

// FIRInstallationsStoredIIDCheckin.m
NSString *const kFIRInstallationsMessageCodeIIDCheckinCoderVersionMismatch = @"I-FIS007000";
NSString *const kFIRInstallationsMessageCodeIIDCheckinFailedToDecode = @"I-FIS007001";

// FIRInstallations.m
NSString *const kFIRInstallationsMessageCodeInvalidFirebaseAppOptions = @"I-FIS008000";

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

#import <FirebaseCore/FIRLogger.h>

extern FIRLoggerService kFIRLoggerInstallations;

// FIRInstallationsAPIService.m
extern NSString *const kFIRInstallationsMessageCodeSendAPIRequest;
extern NSString *const kFIRInstallationsMessageCodeAPIRequestNetworkError;
extern NSString *const kFIRInstallationsMessageCodeAPIRequestResponse;
extern NSString *const kFIRInstallationsMessageCodeUnexpectedAPIRequestResponse;
extern NSString *const kFIRInstallationsMessageCodeParsingAPIResponse;
extern NSString *const kFIRInstallationsMessageCodeAPIResponseParsingInstallationFailed;
extern NSString *const kFIRInstallationsMessageCodeAPIResponseParsingInstallationSucceed;
extern NSString *const kFIRInstallationsMessageCodeAPIResponseParsingAuthTokenFailed;
extern NSString *const kFIRInstallationsMessageCodeAPIResponseParsingAuthTokenSucceed;

// FIRInstallationsIDController.m
extern NSString *const kFIRInstallationsMessageCodeNewGetInstallationOperationCreated;
extern NSString *const kFIRInstallationsMessageCodeNewGetAuthTokenOperationCreated;
extern NSString *const kFIRInstallationsMessageCodeNewDeleteInstallationOperationCreated;
extern NSString *const kFIRInstallationsMessageCodeInvalidFirebaseConfiguration;

// FIRInstallationsStoredItem.m
extern NSString *const kFIRInstallationsMessageCodeInstallationCoderVersionMismatch;

// FIRInstallationsStoredAuthToken.m
extern NSString *const kFIRInstallationsMessageCodeAuthTokenCoderVersionMismatch;

// FIRInstallationsStoredIIDCheckin.m
extern NSString *const kFIRInstallationsMessageCodeIIDCheckinCoderVersionMismatch;
extern NSString *const kFIRInstallationsMessageCodeIIDCheckinFailedToDecode;

// FIRInstallations.m
extern NSString *const kFIRInstallationsMessageCodeInvalidFirebaseAppOptions;

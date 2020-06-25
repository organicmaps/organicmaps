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

#include "FIRCLSApplicationIdentifierModel.h"
#include "FIRCLSProfiling.h"
#include "FIRCrashlytics.h"

@class FBLPromise<T>;

NS_ASSUME_NONNULL_BEGIN

@class FIRCLSDataCollectionArbiter;
@class FIRCLSFileManager;
@class FIRCLSInternalReport;
@class FIRCLSSettings;
@class GDTCORTransport;
@class FIRInstallations;
@protocol FIRAnalyticsInterop;

@interface FIRCLSReportManager : NSObject

- (instancetype)initWithFileManager:(FIRCLSFileManager *)fileManager
                      installations:(FIRInstallations *)installations
                          analytics:(nullable id<FIRAnalyticsInterop>)analytics
                        googleAppID:(NSString *)googleAppID
                        dataArbiter:(FIRCLSDataCollectionArbiter *)dataArbiter
                    googleTransport:(GDTCORTransport *)googleTransport
                         appIDModel:(FIRCLSApplicationIdentifierModel *)appIDModel
                           settings:(FIRCLSSettings *)settings NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (FBLPromise<NSNumber *> *)startWithProfilingMark:(FIRCLSProfileMark)mark;

- (FBLPromise<NSNumber *> *)checkForUnsentReports;
- (FBLPromise *)sendUnsentReports;
- (FBLPromise *)deleteUnsentReports;

@end

extern NSString *const FIRCLSConfigSubmitReportsKey;
extern NSString *const FIRCLSConfigPackageReportsKey;

NS_ASSUME_NONNULL_END

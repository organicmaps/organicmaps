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

#import <GoogleDataTransport/GDTCORTransport.h>

@class FIRCLSDataCollectionToken;
@class FIRCLSInternalReport;
@class FIRCLSSettings;
@class FIRCLSFileManager;
@class FIRCLSNetworkClient;
@class FIRCLSReportUploader;

@protocol FIRCLSReportUploaderDelegate;
@protocol FIRCLSReportUploaderDataSource;
@protocol FIRAnalyticsInterop;

@interface FIRCLSReportUploader : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithQueue:(NSOperationQueue *)queue
                     delegate:(id<FIRCLSReportUploaderDelegate>)delegate
                   dataSource:(id<FIRCLSReportUploaderDataSource>)dataSource
                       client:(FIRCLSNetworkClient *)client
                  fileManager:(FIRCLSFileManager *)fileManager
                    analytics:(id<FIRAnalyticsInterop>)analytics NS_DESIGNATED_INITIALIZER;

@property(nonatomic, weak) id<FIRCLSReportUploaderDelegate> delegate;
@property(nonatomic, weak) id<FIRCLSReportUploaderDataSource> dataSource;

@property(nonatomic, readonly) NSOperationQueue *operationQueue;
@property(nonatomic, readonly) FIRCLSNetworkClient *networkClient;
@property(nonatomic, readonly) FIRCLSFileManager *fileManager;

- (BOOL)prepareAndSubmitReport:(FIRCLSInternalReport *)report
           dataCollectionToken:(FIRCLSDataCollectionToken *)dataCollectionToken
                      asUrgent:(BOOL)urgent
                withProcessing:(BOOL)shouldProcess;

- (BOOL)uploadPackagedReportAtPath:(NSString *)path
               dataCollectionToken:(FIRCLSDataCollectionToken *)dataCollectionToken
                          asUrgent:(BOOL)urgent;

- (void)reportUploadAtPath:(NSString *)path
       dataCollectionToken:(FIRCLSDataCollectionToken *)dataCollectionToken
        completedWithError:(NSError *)error;

@end

@protocol FIRCLSReportUploaderDelegate <NSObject>
@required

- (void)didCompletePackageSubmission:(NSString *)path
                 dataCollectionToken:(FIRCLSDataCollectionToken *)token
                               error:(NSError *)error;
- (void)didCompleteAllSubmissions;

@end

@protocol FIRCLSReportUploaderDataSource <NSObject>
@required

- (NSString *)googleAppID;
- (FIRCLSSettings *)settings;
- (GDTCORTransport *)googleTransport;

@end

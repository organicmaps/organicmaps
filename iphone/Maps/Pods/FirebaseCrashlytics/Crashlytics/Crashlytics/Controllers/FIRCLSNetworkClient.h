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

typedef NS_ENUM(NSInteger, FIRCLSNetworkClientErrorType) {
  FIRCLSNetworkClientErrorTypeUnknown = -1,
  FIRCLSNetworkClientErrorTypeFileUnreadable = -2
};

extern NSString *const FIRCLSNetworkClientErrorDomain;

@protocol FIRCLSNetworkClientDelegate;
@class FIRCLSDataCollectionToken;
@class FIRCLSFileManager;

@interface FIRCLSNetworkClient : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithQueue:(NSOperationQueue *)queue
                  fileManager:(FIRCLSFileManager *)fileManager
                     delegate:(id<FIRCLSNetworkClientDelegate>)delegate;

@property(nonatomic, weak) id<FIRCLSNetworkClientDelegate> delegate;

@property(nonatomic, readonly) NSOperationQueue *operationQueue;
@property(nonatomic, readonly) BOOL supportsBackgroundRequests;

- (void)startUploadRequest:(NSURLRequest *)request
                  filePath:(NSString *)path
       dataCollectionToken:(FIRCLSDataCollectionToken *)dataCollectionToken
               immediately:(BOOL)immediate;

- (void)attemptToReconnectBackgroundSessionWithCompletionBlock:(void (^)(void))completionBlock;

@end

@class FIRCLSNetworkClient;

@protocol FIRCLSNetworkClientDelegate <NSObject>
@required
- (BOOL)networkClientCanUseBackgroundSessions:(FIRCLSNetworkClient *)client;

@optional
- (void)networkClient:(FIRCLSNetworkClient *)client
    didFinishUploadWithPath:(NSString *)path
                      error:(NSError *)error;

@end

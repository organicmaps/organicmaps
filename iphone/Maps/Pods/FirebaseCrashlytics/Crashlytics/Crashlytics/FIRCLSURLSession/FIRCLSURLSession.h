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

#import "FIRCLSURLSessionAvailability.h"

#if FIRCLSURLSESSION_REQUIRED

#import "FIRCLSURLSessionConfiguration.h"

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface FIRCLSURLSession : NSObject {
  id<NSURLSessionDelegate> _delegate;
  NSOperationQueue *_delegateQueue;
  NSURLSessionConfiguration *_configuration;
  NSMutableSet *_taskSet;
  dispatch_queue_t _queue;

  NSString *_sessionDescription;
}

+ (BOOL)NSURLSessionShouldBeUsed;

+ (NSURLSession *)sessionWithConfiguration:(NSURLSessionConfiguration *)configuration;
+ (NSURLSession *)sessionWithConfiguration:(NSURLSessionConfiguration *)configuration
                                  delegate:(nullable id<NSURLSessionDelegate>)delegate
                             delegateQueue:(nullable NSOperationQueue *)queue;

@property(nonatomic, readonly, retain) NSOperationQueue *delegateQueue;
@property(nonatomic, readonly, retain) id<NSURLSessionDelegate> delegate;
@property(nonatomic, readonly, copy) NSURLSessionConfiguration *configuration;

@property(nonatomic, copy) NSString *sessionDescription;

- (void)getTasksWithCompletionHandler:
    (void (^)(NSArray *dataTasks, NSArray *uploadTasks, NSArray *downloadTasks))completionHandler;

// task creation - suitable for background operations
- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request fromFile:(NSURL *)fileURL;

- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request;
- (NSURLSessionDownloadTask *)downloadTaskWithURL:(NSURL *)url;

// convenience methods (that are not available for background sessions
- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
                            completionHandler:(nullable void (^)(NSData *data,
                                                                 NSURLResponse *response,
                                                                 NSError *error))completionHandler;
- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request;

- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request
                                    completionHandler:
                                        (nullable void (^)(NSURL *targetPath,
                                                           NSURLResponse *response,
                                                           NSError *error))completionHandler;

- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request
                                         fromFile:(NSURL *)fileURL
                                completionHandler:
                                    (nullable void (^)(NSData *data,
                                                       NSURLResponse *response,
                                                       NSError *error))completionHandler;

- (void)invalidateAndCancel;
- (void)finishTasksAndInvalidate;

@end

NS_ASSUME_NONNULL_END

#endif

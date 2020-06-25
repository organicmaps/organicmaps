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

OBJC_EXTERN const NSUInteger FIRCLSNetworkMaximumRetryCount;

NS_ASSUME_NONNULL_BEGIN

typedef void (^FIRCLSNetworkDataTaskCompletionHandlerBlock)(NSData *__nullable data,
                                                            NSURLResponse *__nullable response,
                                                            NSError *__nullable error);
typedef void (^FIRCLSNetworkDownloadTaskCompletionHandlerBlock)(NSURL *__nullable location,
                                                                NSURLResponse *__nullable response,
                                                                NSError *__nullable error);

@interface FIRCLSFABNetworkClient : NSObject

- (instancetype)init;
- (instancetype)initWithQueue:(nullable NSOperationQueue *)operationQueue;
- (instancetype)initWithSessionConfiguration:(NSURLSessionConfiguration *)config
                                       queue:(nullable NSOperationQueue *)operationQueue
    NS_DESIGNATED_INITIALIZER;

- (void)startDataTaskWithRequest:(NSURLRequest *)request
                      retryLimit:(NSUInteger)retryLimit
               completionHandler:(FIRCLSNetworkDataTaskCompletionHandlerBlock)completionHandler;
- (void)startDownloadTaskWithRequest:(NSURLRequest *)request
                          retryLimit:(NSUInteger)retryLimit
                   completionHandler:
                       (FIRCLSNetworkDownloadTaskCompletionHandlerBlock)completionHandler;

- (void)invalidateAndCancel;

// Backwards compatibility (we cannot change an interface in Fabric Base that other kits rely on,
// since we have no control of versioning dependencies)
- (void)startDataTaskWithRequest:(NSURLRequest *)request
               completionHandler:(FIRCLSNetworkDataTaskCompletionHandlerBlock)completionHandler;
- (void)startDownloadTaskWithRequest:(NSURLRequest *)request
                   completionHandler:
                       (FIRCLSNetworkDownloadTaskCompletionHandlerBlock)completionHandler;

@end

NS_ASSUME_NONNULL_END

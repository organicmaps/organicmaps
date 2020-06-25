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

#import "FIRCLSFABNetworkClient.h"

#if FIRCLSURLSESSION_REQUIRED
#import "FIRCLSURLSession.h"
#endif

#import "FIRCLSNetworkResponseHandler.h"

static const float FIRCLSNetworkMinimumRetryJitter = 0.90f;
static const float FIRCLSNetworkMaximumRetryJitter = 1.10f;
const NSUInteger FIRCLSNetworkMaximumRetryCount = 10;

@interface FIRCLSFABNetworkClient () <NSURLSessionDelegate, NSURLSessionTaskDelegate>

@property(nonatomic, strong, readonly) NSURLSession *session;

@end

@implementation FIRCLSFABNetworkClient

- (instancetype)init {
  return [self initWithQueue:nil];
}

- (instancetype)initWithQueue:(nullable NSOperationQueue *)operationQueue {
#if !FIRCLSURLSESSION_REQUIRED
  NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
#else
  NSURLSessionConfiguration *config = [FIRCLSURLSessionConfiguration defaultSessionConfiguration];
#endif
  return [self initWithSessionConfiguration:config queue:operationQueue];
}

- (instancetype)initWithSessionConfiguration:(NSURLSessionConfiguration *)config
                                       queue:(nullable NSOperationQueue *)operationQueue {
  self = [super init];
  if (!self) {
    return nil;
  }

#if !FIRCLSURLSESSION_REQUIRED
  _session = [NSURLSession sessionWithConfiguration:config
                                           delegate:self
                                      delegateQueue:operationQueue];
#else
  _session = [FIRCLSURLSession sessionWithConfiguration:config
                                               delegate:self
                                          delegateQueue:operationQueue];
#endif
  if (!_session) {
    return nil;
  }

  return self;
}

- (void)dealloc {
  [_session finishTasksAndInvalidate];
}

#pragma mark - Delay Handling
- (double)randomDoubleWithMin:(double)min max:(double)max {
  return min + ((max - min) * drand48());
}

- (double)generateRandomJitter {
  return [self randomDoubleWithMin:FIRCLSNetworkMinimumRetryJitter
                               max:FIRCLSNetworkMaximumRetryJitter];
}

- (NSTimeInterval)computeDelayForResponse:(NSURLResponse *)response
                           withRetryCount:(NSUInteger)count {
  NSTimeInterval initialValue = [FIRCLSNetworkResponseHandler retryValueForResponse:response];

  // make sure count is > 0
  count = MAX(count, 1);
  // make sure initialValue is >2 for exponential backoff to work reasonably with low count numbers
  initialValue = MAX(initialValue, 2.0);

  const double jitter = [self generateRandomJitter];

  return pow(initialValue, count) * jitter;  // exponential backoff
}

- (void)runAfterRetryValueFromResponse:(NSURLResponse *)response
                              attempts:(NSUInteger)count
                               onQueue:(dispatch_queue_t)queue
                                 block:(void (^)(void))block {
  const NSTimeInterval delay = [self computeDelayForResponse:response withRetryCount:count];

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (uint64_t)(delay * NSEC_PER_SEC)), queue, block);
}

- (void)runAfterRetryValueFromResponse:(NSURLResponse *)response
                              attempts:(NSUInteger)count
                                 block:(void (^)(void))block {
  dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  [self runAfterRetryValueFromResponse:response attempts:count onQueue:queue block:block];
}

#pragma mark - Tasks

- (void)startDataTaskWithRequest:(NSURLRequest *)request
                      retryLimit:(NSUInteger)retryLimit
                           tries:(NSUInteger)tries
               completionHandler:(FIRCLSNetworkDataTaskCompletionHandlerBlock)completionHandler {
  NSURLSessionTask *task = [self.session
      dataTaskWithRequest:request
        completionHandler:^(NSData *data, NSURLResponse *response, NSError *taskError) {
          [FIRCLSNetworkResponseHandler
              handleCompletedResponse:response
                   forOriginalRequest:request
                                error:taskError
                                block:^(BOOL retry, NSError *error) {
                                  if (!retry) {
                                    completionHandler(data, response, error);
                                    return;
                                  }

                                  if (tries >= retryLimit) {
                                    NSDictionary *userInfo = @{
                                      @"retryLimit" : @(retryLimit),
                                      NSURLErrorFailingURLStringErrorKey : request.URL
                                    };
                                    completionHandler(
                                        nil, nil,
                                        [NSError
                                            errorWithDomain:FIRCLSNetworkErrorDomain
                                                       code:FIRCLSNetworkErrorMaximumAttemptsReached
                                                   userInfo:userInfo]);
                                    return;
                                  }

                                  [self
                                      runAfterRetryValueFromResponse:response
                                                            attempts:tries
                                                               block:^{
                                                                 [self
                                                                     startDataTaskWithRequest:
                                                                         request
                                                                                   retryLimit:
                                                                                       retryLimit
                                                                                        tries:
                                                                                            (tries +
                                                                                             1)
                                                                            completionHandler:
                                                                                completionHandler];
                                                               }];
                                }];
        }];

  [task resume];

  if (!task) {
    completionHandler(nil, nil,
                      [NSError errorWithDomain:FIRCLSNetworkErrorDomain
                                          code:FIRCLSNetworkErrorFailedToStartOperation
                                      userInfo:nil]);
  }
}

- (void)startDataTaskWithRequest:(NSURLRequest *)request
                      retryLimit:(NSUInteger)retryLimit
               completionHandler:(FIRCLSNetworkDataTaskCompletionHandlerBlock)completionHandler {
  [self startDataTaskWithRequest:request
                      retryLimit:retryLimit
                           tries:0
               completionHandler:completionHandler];
}

- (void)startDataTaskWithRequest:(NSURLRequest *)request
               completionHandler:(FIRCLSNetworkDataTaskCompletionHandlerBlock)completionHandler {
  [self startDataTaskWithRequest:request
                      retryLimit:FIRCLSNetworkMaximumRetryCount
               completionHandler:completionHandler];
}

- (void)startDownloadTaskWithRequest:(NSURLRequest *)request
                          retryLimit:(NSUInteger)retryLimit
                               tries:(NSUInteger)tries
                   completionHandler:
                       (FIRCLSNetworkDownloadTaskCompletionHandlerBlock)completionHandler {
  NSURLSessionTask *task = [self.session
      downloadTaskWithRequest:request
            completionHandler:^(NSURL *location, NSURLResponse *response, NSError *taskError) {
              [FIRCLSNetworkResponseHandler
                  handleCompletedResponse:response
                       forOriginalRequest:request
                                    error:taskError
                                    block:^(BOOL retry, NSError *error) {
                                      if (!retry) {
                                        completionHandler(location, response, error);
                                        return;
                                      }

                                      if (tries >= retryLimit) {
                                        NSDictionary *userInfo = @{
                                          @"retryLimit" : @(retryLimit),
                                          NSURLErrorFailingURLStringErrorKey : request.URL
                                        };
                                        completionHandler(
                                            nil, nil,
                                            [NSError
                                                errorWithDomain:FIRCLSNetworkErrorDomain
                                                           code:
                                                               FIRCLSNetworkErrorMaximumAttemptsReached
                                                       userInfo:userInfo]);
                                        return;
                                      }

                                      [self
                                          runAfterRetryValueFromResponse:response
                                                                attempts:tries
                                                                   block:^{
                                                                     [self
                                                                         startDownloadTaskWithRequest:
                                                                             request
                                                                                           retryLimit:
                                                                                               retryLimit
                                                                                                tries:
                                                                                                    (tries +
                                                                                                     1)
                                                                                    completionHandler:
                                                                                        completionHandler];
                                                                   }];
                                    }];
            }];

  [task resume];

  if (!task) {
    completionHandler(nil, nil,
                      [NSError errorWithDomain:FIRCLSNetworkErrorDomain
                                          code:FIRCLSNetworkErrorFailedToStartOperation
                                      userInfo:nil]);
  }
}

- (void)startDownloadTaskWithRequest:(NSURLRequest *)request
                          retryLimit:(NSUInteger)retryLimit
                   completionHandler:
                       (FIRCLSNetworkDownloadTaskCompletionHandlerBlock)completionHandler {
  [self startDownloadTaskWithRequest:request
                          retryLimit:retryLimit
                               tries:0
                   completionHandler:completionHandler];
}

- (void)startDownloadTaskWithRequest:(NSURLRequest *)request
                   completionHandler:
                       (FIRCLSNetworkDownloadTaskCompletionHandlerBlock)completionHandler {
  [self startDownloadTaskWithRequest:request
                          retryLimit:FIRCLSNetworkMaximumRetryCount
                   completionHandler:completionHandler];
}

- (void)invalidateAndCancel {
  [self.session invalidateAndCancel];
}

#pragma mark - NSURLSession Delegate
- (void)URLSession:(NSURLSession *)session didBecomeInvalidWithError:(NSError *)error {
}

@end

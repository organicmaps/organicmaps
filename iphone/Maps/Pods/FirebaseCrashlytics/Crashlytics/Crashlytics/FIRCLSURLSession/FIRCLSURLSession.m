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
#import "FIRCLSURLSession.h"

#import "FIRCLSURLSessionDataTask.h"
#import "FIRCLSURLSessionDataTask_PrivateMethods.h"
#import "FIRCLSURLSessionDownloadTask.h"
#import "FIRCLSURLSessionDownloadTask_PrivateMethods.h"
#import "FIRCLSURLSessionTask_PrivateMethods.h"
#import "FIRCLSURLSessionUploadTask.h"

#define DELEGATE ((id<NSURLSessionDataDelegate, NSURLSessionDownloadDelegate>)self->_delegate)

@interface FIRCLSURLSession () <FIRCLSURLSessionDownloadDelegate>

@property(nonatomic, retain) NSOperationQueue *delegateQueue;
@property(nonatomic, retain) id<NSURLSessionDelegate> delegate;
@property(nonatomic, copy) NSURLSessionConfiguration *configuration;

@end

@implementation FIRCLSURLSession

@synthesize delegate = _delegate;
@synthesize delegateQueue = _delegateQueue;
@synthesize configuration = _configuration;
@synthesize sessionDescription = _sessionDescription;

+ (BOOL)NSURLSessionShouldBeUsed {
  if (!NSClassFromString(@"NSURLSession")) {
    return NO;
  }

  // We use this as a proxy to verify that we are on at least iOS 8 or 10.10. The first OSes that
  // has NSURLSession were fairly unstable.
  return [[NSURLSessionConfiguration class]
      respondsToSelector:@selector(backgroundSessionConfigurationWithIdentifier:)];
}

+ (NSURLSession *)sessionWithConfiguration:(NSURLSessionConfiguration *)configuration {
  return [self sessionWithConfiguration:configuration delegate:nil delegateQueue:nil];
}

+ (NSURLSession *)sessionWithConfiguration:(NSURLSessionConfiguration *)configuration
                                  delegate:(nullable id<NSURLSessionDelegate>)delegate
                             delegateQueue:(nullable NSOperationQueue *)queue {
  if ([self NSURLSessionShouldBeUsed]) {
    return [NSURLSession sessionWithConfiguration:configuration
                                         delegate:delegate
                                    delegateQueue:queue];
  }

  if (!configuration) {
    return nil;
  }

#if __has_feature(objc_arc)
  FIRCLSURLSession *session = [self new];
#else
  FIRCLSURLSession *session = [[self new] autorelease];
#endif
  [session setDelegate:delegate];
  // When delegate exists, but delegateQueue is nil, create a serial queue like NSURLSession
  // documents.
  if (delegate && !queue) {
    queue = [self newDefaultDelegateQueue];
  }
  session.delegateQueue = queue;
  session.configuration = configuration;
  return (NSURLSession *)session;
}

+ (NSOperationQueue *)newDefaultDelegateQueue {
  NSOperationQueue *delegateQueue = [[NSOperationQueue alloc] init];
  delegateQueue.name = [NSString stringWithFormat:@"%@ %p", NSStringFromClass(self), self];
  delegateQueue.maxConcurrentOperationCount = 1;
  return delegateQueue;
}

- (instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  _queue = dispatch_queue_create("com.crashlytics.URLSession", 0);

  return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc {
  [_taskSet release];
  [_delegate release];
  [_delegateQueue release];
  [_configuration release];

#if !OS_OBJECT_USE_OBJC
  dispatch_release(_queue);
#endif

  [super dealloc];
}
#endif

#pragma mark - Managing the Session

- (void)invalidateAndCancel {
  dispatch_sync(_queue, ^{
    for (FIRCLSURLSessionTask *task in self->_taskSet) {
      [task cancel];
    }
  });

  self.delegate = nil;
}

- (void)finishTasksAndInvalidate {
  self.delegate = nil;
}

#pragma mark -

- (void)getTasksWithCompletionHandler:
    (void (^)(NSArray *dataTasks, NSArray *uploadTasks, NSArray *downloadTasks))completionHandler {
  [[self delegateQueue] addOperationWithBlock:^{
    // TODO - this is totally wrong, but better than not calling back at all
    completionHandler(@[], @[], @[]);
  }];
}

- (void)removeTaskFromSet:(FIRCLSURLSessionTask *)task {
  dispatch_async(_queue, ^{
    [self->_taskSet removeObject:task];
  });
}

- (void)configureTask:(FIRCLSURLSessionTask *)task
          withRequest:(NSURLRequest *)request
                block:(void (^)(NSMutableURLRequest *mutableRequest))block {
  NSMutableURLRequest *modifiedRequest = [request mutableCopy];

  dispatch_sync(_queue, ^{
    [self->_taskSet addObject:task];

    // TODO: this isn't allowed to overwrite existing headers
    for (NSString *key in [self->_configuration HTTPAdditionalHeaders]) {
      [modifiedRequest addValue:[[self->_configuration HTTPAdditionalHeaders] objectForKey:key]
             forHTTPHeaderField:key];
    }
  });

  if (block) {
    block(modifiedRequest);
  }

  [task setOriginalRequest:modifiedRequest];
  [task setDelegate:self];

#if !__has_feature(objc_arc)
  [modifiedRequest release];
#endif
}

- (BOOL)shouldInvokeDelegateSelector:(SEL)selector forTask:(FIRCLSURLSessionTask *)task {
  return [task invokesDelegate] && [_delegate respondsToSelector:selector];
}

#pragma mark Task Creation
- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request
                                         fromFile:(NSURL *)fileURL {
  return [self uploadTaskWithRequest:request fromFile:fileURL completionHandler:nil];
}

- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request {
  return [self downloadTaskWithRequest:request completionHandler:nil];
}

- (NSURLSessionDownloadTask *)downloadTaskWithURL:(NSURL *)url {
  return [self downloadTaskWithRequest:[NSURLRequest requestWithURL:url]];
}

#pragma mark Async Convenience Methods
- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
                            completionHandler:(nullable void (^)(NSData *data,
                                                                 NSURLResponse *response,
                                                                 NSError *error))completionHandler {
  FIRCLSURLSessionDataTask *task = [FIRCLSURLSessionDataTask task];

  if (completionHandler) {
    [task setCompletionHandler:completionHandler];
    [task setInvokesDelegate:NO];
  }

  [self configureTask:task withRequest:request block:nil];

  return (NSURLSessionDataTask *)task;
}

- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request {
  return [self dataTaskWithRequest:request completionHandler:nil];
}

- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request
                                         fromFile:(NSURL *)fileURL
                                completionHandler:
                                    (nullable void (^)(NSData *data,
                                                       NSURLResponse *response,
                                                       NSError *error))completionHandler {
  FIRCLSURLSessionUploadTask *task = [FIRCLSURLSessionUploadTask task];

  if (completionHandler) {
    [task setCompletionHandler:completionHandler];
    [task setInvokesDelegate:NO];
  }

  [self configureTask:task
          withRequest:request
                block:^(NSMutableURLRequest *mutableRequest) {
                  // you cannot set up both of these, and we'll be using the stream here
                  [mutableRequest setHTTPBody:nil];
                  [mutableRequest setHTTPBodyStream:[NSInputStream inputStreamWithURL:fileURL]];
                }];

  return (NSURLSessionUploadTask *)task;
}

- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request
                                    completionHandler:
                                        (nullable void (^)(NSURL *targetPath,
                                                           NSURLResponse *response,
                                                           NSError *error))completionHandler {
  FIRCLSURLSessionDownloadTask *task = [FIRCLSURLSessionDownloadTask task];

  if (completionHandler) {
    [task setDownloadCompletionHandler:completionHandler];
    [task setInvokesDelegate:NO];
  }

  [self configureTask:task withRequest:request block:nil];

  return (NSURLSessionDownloadTask *)task;
}

#pragma mark FIRCLSURLSessionTaskDelegate
- (NSURLRequest *)task:(FIRCLSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response
                    newRequest:(NSURLRequest *)request {
  // just accept the proposed redirection
  return request;
}

- (void)task:(FIRCLSURLSessionTask *)task didCompleteWithError:(NSError *)error {
  if (![self shouldInvokeDelegateSelector:@selector(URLSession:task:didCompleteWithError:)
                                  forTask:task]) {
    [self removeTaskFromSet:task];
    return;
  }

  [_delegateQueue addOperationWithBlock:^{
    [DELEGATE URLSession:(NSURLSession *)self
                        task:(NSURLSessionTask *)task
        didCompleteWithError:error];

    // Note that you *cannot* clean up here, because this method could be run asynchronously with
    // the delegate methods that care about the state of the task
    [self removeTaskFromSet:task];
  }];
}

#pragma mark FIRCLSURLSessionDataTask
- (void)task:(FIRCLSURLSessionDataTask *)task didReceiveResponse:(NSURLResponse *)response {
  if (![self shouldInvokeDelegateSelector:@selector
             (URLSession:dataTask:didReceiveResponse:completionHandler:)
                                  forTask:task]) {
    return;
  }

  [_delegateQueue addOperationWithBlock:^{
    [DELEGATE URLSession:(NSURLSession *)self
                  dataTask:(NSURLSessionDataTask *)task
        didReceiveResponse:response
         completionHandler:^(NSURLSessionResponseDisposition disposition){
             // nothing to do here
         }];
  }];
}

- (void)task:(FIRCLSURLSessionDataTask *)task didReceiveData:(NSData *)data {
  if (![self shouldInvokeDelegateSelector:@selector(URLSession:dataTask:didReceiveData:)
                                  forTask:task]) {
    return;
  }

  [_delegateQueue addOperationWithBlock:^{
    [DELEGATE URLSession:(NSURLSession *)self
                dataTask:(NSURLSessionDataTask *)task
          didReceiveData:data];
  }];
}

#pragma mark FIRCLSURLSessionDownloadDelegate
- (void)downloadTask:(FIRCLSURLSessionDownloadTask *)task didFinishDownloadingToURL:(NSURL *)url {
  if (![self shouldInvokeDelegateSelector:@selector(URLSession:
                                                  downloadTask:didFinishDownloadingToURL:)
                                  forTask:task]) {
    // We have to be certain that we cleanup only once the delegate no longer cares about the state
    // of the task being changed.  In the case of download, this is either after the delegate method
    // has been invoked, or here, if the delegate doesn't care.
    [task cleanup];
    return;
  }

  [_delegateQueue addOperationWithBlock:^{
    [DELEGATE URLSession:(NSURLSession *)self
                     downloadTask:(NSURLSessionDownloadTask *)task
        didFinishDownloadingToURL:url];

    // Cleanup for the download tasks is a little complex.  As long as we do it only after
    // the delegate has been informed of the completed download, we are ok.
    [task cleanup];
  }];
}

@end

#else

INJECT_STRIP_SYMBOL(clsurlsession)

#endif

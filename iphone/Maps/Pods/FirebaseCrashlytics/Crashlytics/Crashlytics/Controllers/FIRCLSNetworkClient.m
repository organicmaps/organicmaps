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

#import "FIRCLSNetworkClient.h"

#import "FIRCLSApplication.h"
#import "FIRCLSByteUtility.h"
#import "FIRCLSDataCollectionToken.h"
#import "FIRCLSDefines.h"
#import "FIRCLSFileManager.h"
#import "FIRCLSNetworkResponseHandler.h"
#import "FIRCLSURLSession.h"
#import "FIRCLSURLSessionConfiguration.h"

#import "FIRCLSUtility.h"

NSString *const FIRCLSNetworkClientErrorDomain = @"FIRCLSNetworkError";

NSString *const FIRCLSNetworkClientBackgroundIdentifierSuffix = @".crash.background-session";

@interface FIRCLSNetworkClient () <NSURLSessionDelegate> {
  NSURLSession *_session;
}

@property(nonatomic, strong) void (^backgroundCompletionHandler)(void);
@property(nonatomic, strong, readonly) NSURLSession *session;
@property(nonatomic, assign) BOOL canUseBackgroundSession;
@property(nonatomic, strong) FIRCLSFileManager *fileManager;

@end

@implementation FIRCLSNetworkClient

- (instancetype)initWithQueue:(NSOperationQueue *)queue
                  fileManager:(FIRCLSFileManager *)fileManager
                     delegate:(id<FIRCLSNetworkClientDelegate>)delegate {
  self = [super init];
  if (!self) {
    return nil;
  }

  _operationQueue = queue;
  _delegate = delegate;
  _fileManager = fileManager;

  self.canUseBackgroundSession = [_delegate networkClientCanUseBackgroundSessions:self];

  if (!self.supportsBackgroundRequests) {
    FIRCLSDeveloperLog(
        "Crashlytics:Crash:Client",
        @"Background session uploading not supported, asynchronous uploading will be used");
  }

  return self;
}

#pragma mark - Background Support
- (NSURLSession *)session {
  // Creating a NSURLSession takes some time. Doing it lazily saves us time in the normal case.
  if (_session) {
    return _session;
  }

  NSURLSessionConfiguration *config = nil;

  Class urlSessionClass;
  Class urlSessionConfigurationClass;
#if FIRCLSURLSESSION_REQUIRED
  urlSessionClass = [FIRCLSURLSession class];
  urlSessionConfigurationClass = [FIRCLSURLSessionConfiguration class];
#else
  urlSessionClass = [NSURLSession class];
  urlSessionConfigurationClass = [NSURLSessionConfiguration class];
#endif

  if (self.supportsBackgroundRequests) {
    NSString *sdkBundleID = FIRCLSApplicationGetSDKBundleID();
    NSString *backgroundConfigName =
        [sdkBundleID stringByAppendingString:FIRCLSNetworkClientBackgroundIdentifierSuffix];

    config = [urlSessionConfigurationClass backgroundSessionConfiguration:backgroundConfigName];
#if TARGET_OS_IPHONE
    [config setSessionSendsLaunchEvents:NO];
#endif
  }

  if (!config) {
    // take this code path if we don't support background requests OR if we failed to create a
    // background configuration
    config = [urlSessionConfigurationClass defaultSessionConfiguration];
  }

  _session = [urlSessionClass sessionWithConfiguration:config
                                              delegate:self
                                         delegateQueue:self.operationQueue];

  if (!_session || !config) {
    FIRCLSErrorLog(@"Failed to initialize");
  }

  return _session;
}

#if FIRCLSURLSESSION_REQUIRED
- (BOOL)NSURLSessionAvailable {
  if ([[FIRCLSURLSession class] respondsToSelector:@selector(NSURLSessionShouldBeUsed)]) {
    return [FIRCLSURLSession NSURLSessionShouldBeUsed];
  }

  return NSClassFromString(@"NSURLSession") != nil;
}
#endif

- (BOOL)supportsBackgroundRequests {
  return !FIRCLSApplicationIsExtension()
#if FIRCLSURLSESSION_REQUIRED
         && [self NSURLSessionAvailable]
#endif
         && self.canUseBackgroundSession;
}

- (void)attemptToReconnectBackgroundSessionWithCompletionBlock:(void (^)(void))completionBlock {
  if (!self.supportsBackgroundRequests) {
    if (completionBlock) {
      completionBlock();
    }
    return;
  }

  // This is the absolute minimum necessary. Perhaps we can do better?
  if (completionBlock) {
    [[NSOperationQueue mainQueue] addOperationWithBlock:completionBlock];
  }
}

#pragma mark - API
- (void)startUploadRequest:(NSURLRequest *)request
                  filePath:(NSString *)path
       dataCollectionToken:(FIRCLSDataCollectionToken *)dataCollectionToken
               immediately:(BOOL)immediate {
  if (![dataCollectionToken isValid]) {
    FIRCLSErrorLog(@"An upload was requested with an invalid data collection token.");
    return;
  }

  if (immediate) {
    [self startImmediateUploadRequest:request filePath:path];
    return;
  }

  NSString *description = [self relativeTaskPathForAbsolutePath:path];
  [self checkForExistingTaskMatchingDescription:description
                                completionBlock:^(BOOL found) {
                                  if (found) {
                                    FIRCLSDeveloperLog(
                                        "Crashlytics:Crash:Client",
                                        @"A task currently exists for this upload, skipping");
                                    return;
                                  }

                                  [self startNewUploadRequest:request filePath:path];
                                }];
}

#pragma mark - Support API
- (void)startImmediateUploadRequest:(NSURLRequest *)request filePath:(NSString *)path {
  // check the ivar directly, to avoid going back to the delegate
  if (self.supportsBackgroundRequests) {
    // this can be done here, because the request will be started synchronously.
    [self startNewUploadRequest:request filePath:path];
    return;
  }

  if (![[NSFileManager defaultManager] isReadableFileAtPath:path]) {
    FIRCLSSDKLog("Error: file unreadable\n");
    // Following the same logic as below, do not try to inform the delegate
    return;
  }

  NSMutableURLRequest *mutableRequest = [request mutableCopy];

  [mutableRequest setHTTPBodyStream:[NSInputStream inputStreamWithFileAtPath:path]];

  NSURLResponse *requestResponse = nil;

  [[NSURLSession sharedSession]
      dataTaskWithRequest:mutableRequest
        completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                            NSError *_Nullable error) {
          [FIRCLSNetworkResponseHandler
              clientResponseType:requestResponse
                         handler:^(FIRCLSNetworkClientResponseType type, NSInteger statusCode) {
                           if (type != FIRCLSNetworkClientResponseSuccess) {
                             // don't even inform the delegate of a failure here, because we don't
                             // want any action to be taken synchronously
                             return;
                           }

                           [[self delegate] networkClient:self
                                  didFinishUploadWithPath:path
                                                    error:error];
                         }];
        }];
}

- (void)startNewUploadRequest:(NSURLRequest *)request filePath:(NSString *)path {
  if (![[NSFileManager defaultManager] isReadableFileAtPath:path]) {
    [self.operationQueue addOperationWithBlock:^{
      [self
          handleTaskDescription:path
             completedWithError:[NSError errorWithDomain:FIRCLSNetworkClientErrorDomain
                                                    code:FIRCLSNetworkClientErrorTypeFileUnreadable
                                                userInfo:@{@"path" : path}]];
    }];

    return;
  }

  NSURLSessionUploadTask *task = [self.session uploadTaskWithRequest:request
                                                            fromFile:[NSURL fileURLWithPath:path]];

  // set the description, so we can determine what file was successfully uploaded later on
  [task setTaskDescription:[self relativeTaskPathForAbsolutePath:path]];

  [task resume];
}

- (NSString *)rootPath {
  return self.fileManager.rootPath;
}

- (NSString *)absolutePathForRelativeTaskPath:(NSString *)path {
  return [self.rootPath stringByAppendingPathComponent:path];
}

- (NSString *)relativeTaskPathForAbsolutePath:(NSString *)path {
  // make sure this has a tailing slash, so the path looks relative
  NSString *root = [self.rootPath stringByAppendingString:@"/"];

  if (![path hasPrefix:root]) {
    FIRCLSSDKLog("Error: path '%s' is not at the root '%s'", [path UTF8String], [root UTF8String]);
    return nil;
  }

  return [path stringByReplacingOccurrencesOfString:root withString:@""];
}

#pragma mark - Task Management
- (BOOL)taskArray:(NSArray *)array hasTaskMatchingDescription:(NSString *)description {
  NSUInteger idx = [array indexOfObjectPassingTest:^BOOL(id obj, NSUInteger arrayIdx, BOOL *stop) {
    return [[obj taskDescription] isEqualToString:description];
  }];

  return idx != NSNotFound;
}

- (void)checkSession:(NSURLSession *)session
    forTasksMatchingDescription:(NSString *)description
                completionBlock:(void (^)(BOOL found))block {
  if (!session) {
    block(NO);
    return;
  }

  [session getTasksWithCompletionHandler:^(NSArray *dataTasks, NSArray *uploadTasks,
                                           NSArray *downloadTasks) {
    if ([self taskArray:uploadTasks hasTaskMatchingDescription:description]) {
      block(YES);
      return;
    }

    if ([self taskArray:dataTasks hasTaskMatchingDescription:description]) {
      block(YES);
      return;
    }

    if ([self taskArray:downloadTasks hasTaskMatchingDescription:description]) {
      block(YES);
      return;
    }

    block(NO);
  }];
}

- (void)checkForExistingTaskMatchingDescription:(NSString *)description
                                completionBlock:(void (^)(BOOL found))block {
  // Do not instantiate the normal session, because if it doesn't exist yet, it cannot possibly have
  // existing tasks
  [_operationQueue addOperationWithBlock:^{
    [self checkSession:self.session
        forTasksMatchingDescription:description
                    completionBlock:^(BOOL found) {
                      block(found);
                    }];
  }];
}

#pragma mark - Result Handling
// This method is duplicated from FIRCLSFABNetworkClient. Sharing it is a little weird - I didn't
// feel like it fit into FIRCLSNetworkResponseHandler.
- (void)runAfterRetryValueFromResponse:(NSURLResponse *)response block:(void (^)(void))block {
  NSTimeInterval delay = [FIRCLSNetworkResponseHandler retryValueForResponse:response];

  // FIRCLSDeveloperLog("Network", @"Restarting request after %f", delay);

  FIRCLSAddOperationAfter(delay, _operationQueue, block);
}

- (void)restartTask:(NSURLSessionTask *)task {
  NSURLRequest *request = [task originalRequest];

  [self runAfterRetryValueFromResponse:[task response]
                                 block:^{
                                   NSString *path = [self
                                       absolutePathForRelativeTaskPath:[task taskDescription]];

                                   [self startNewUploadRequest:request filePath:path];
                                 }];
}

- (void)handleTask:(NSURLSessionTask *)task completedWithError:(NSError *)error {
  [self handleTaskDescription:[task taskDescription] completedWithError:error];
}

- (void)handleTaskDescription:(NSString *)taskDescription completedWithError:(NSError *)error {
  NSString *path = [self absolutePathForRelativeTaskPath:taskDescription];

  [[self delegate] networkClient:self didFinishUploadWithPath:path error:error];
}

#pragma mark - NSURLSessionDelegate
- (void)URLSession:(NSURLSession *)session didBecomeInvalidWithError:(NSError *)error {
  FIRCLSDeveloperLog("Crashlytics:Crash:Client", @"session became invalid: %@", error);
}

// Careful! Not implementing this method appears to cause a crash when using a background task
- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error {
  [FIRCLSNetworkResponseHandler handleCompletedResponse:task.response
                                     forOriginalRequest:task.originalRequest
                                                  error:error
                                                  block:^(BOOL restart, NSError *taskError) {
                                                    if (restart) {
                                                      [self restartTask:task];
                                                      return;
                                                    }

                                                    [self handleTask:task
                                                        completedWithError:taskError];
                                                  }];
}

@end

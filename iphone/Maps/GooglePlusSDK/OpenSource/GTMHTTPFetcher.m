/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
//  GTMHTTPFetcher.m
//

#define GTMHTTPFETCHER_DEFINE_GLOBALS 1

#import "GTMHTTPFetcher.h"

#if GTM_BACKGROUND_FETCHING
#import <UIKit/UIKit.h>
#endif

static id <GTMCookieStorageProtocol> gGTMFetcherStaticCookieStorage = nil;
static Class gGTMFetcherConnectionClass = nil;

// The default max retry interview is 10 minutes for uploads (POST/PUT/PATCH),
// 1 minute for downloads.
static const NSTimeInterval kUnsetMaxRetryInterval = -1;
static const NSTimeInterval kDefaultMaxDownloadRetryInterval = 60.0;
static const NSTimeInterval kDefaultMaxUploadRetryInterval = 60.0 * 10.;

// delegateQueue callback parameters
static NSString *const kCallbackData = @"data";
static NSString *const kCallbackError = @"error";

//
// GTMHTTPFetcher
//

@interface GTMHTTPFetcher ()

@property (copy) NSString *temporaryDownloadPath;
@property (retain) id <GTMCookieStorageProtocol> cookieStorage;
@property (readwrite, retain) NSData *downloadedData;
#if NS_BLOCKS_AVAILABLE
@property (copy) void (^completionBlock)(NSData *, NSError *);
#endif

- (BOOL)beginFetchMayDelay:(BOOL)mayDelay
              mayAuthorize:(BOOL)mayAuthorize;
- (void)failToBeginFetchWithError:(NSError *)error;
- (void)failToBeginFetchDeferWithError:(NSError *)error;

#if GTM_BACKGROUND_FETCHING
- (void)endBackgroundTask;
- (void)backgroundFetchExpired;
#endif

- (BOOL)authorizeRequest;
- (void)authorizer:(id <GTMFetcherAuthorizationProtocol>)auth
           request:(NSMutableURLRequest *)request
 finishedWithError:(NSError *)error;

- (NSString *)createTempDownloadFilePathForPath:(NSString *)targetPath;
- (void)stopFetchReleasingCallbacks:(BOOL)shouldReleaseCallbacks;
- (BOOL)shouldReleaseCallbacksUponCompletion;

- (void)addCookiesToRequest:(NSMutableURLRequest *)request;
- (void)handleCookiesForResponse:(NSURLResponse *)response;

- (void)invokeFetchCallbacksWithData:(NSData *)data
                               error:(NSError *)error;
- (void)invokeFetchCallback:(SEL)sel
                     target:(id)target
                       data:(NSData *)data
                      error:(NSError *)error;
- (void)invokeFetchCallbacksOnDelegateQueueWithData:(NSData *)data
                                              error:(NSError *)error;
- (void)releaseCallbacks;

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;

- (BOOL)shouldRetryNowForStatus:(NSInteger)status error:(NSError *)error;
- (void)destroyRetryTimer;
- (void)beginRetryTimer;
- (void)primeRetryTimerWithNewTimeInterval:(NSTimeInterval)secs;
- (void)sendStopNotificationIfNeeded;
- (void)retryFetch;
- (void)retryTimerFired:(NSTimer *)timer;
@end

@interface GTMHTTPFetcher (GTMHTTPFetcherLoggingInternal)
- (void)setupStreamLogging;
- (void)logFetchWithError:(NSError *)error;
@end

@implementation GTMHTTPFetcher

+ (GTMHTTPFetcher *)fetcherWithRequest:(NSURLRequest *)request {
  return [[[[self class] alloc] initWithRequest:request] autorelease];
}

+ (GTMHTTPFetcher *)fetcherWithURL:(NSURL *)requestURL {
  return [self fetcherWithRequest:[NSURLRequest requestWithURL:requestURL]];
}

+ (GTMHTTPFetcher *)fetcherWithURLString:(NSString *)requestURLString {
  return [self fetcherWithURL:[NSURL URLWithString:requestURLString]];
}

+ (void)initialize {
  // initialize is guaranteed by the runtime to be called in a
  // thread-safe manner
  if (!gGTMFetcherStaticCookieStorage) {
    Class cookieStorageClass = NSClassFromString(@"GTMCookieStorage");
    if (cookieStorageClass) {
      gGTMFetcherStaticCookieStorage = [[cookieStorageClass alloc] init];
    }
  }
}

- (id)init {
  return [self initWithRequest:nil];
}

- (id)initWithRequest:(NSURLRequest *)request {
  self = [super init];
  if (self) {
    request_ = [request mutableCopy];

    if (gGTMFetcherStaticCookieStorage != nil) {
      // The user has compiled with the cookie storage class available;
      // default to static cookie storage, so our cookies are independent
      // of the cookies of other apps.
      [self setCookieStorageMethod:kGTMHTTPFetcherCookieStorageMethodStatic];
    } else {
      // Default to system default cookie storage
      [self setCookieStorageMethod:kGTMHTTPFetcherCookieStorageMethodSystemDefault];
    }
  }
  return self;
}

- (id)copyWithZone:(NSZone *)zone {
  // disallow use of fetchers in a copy property
  [self doesNotRecognizeSelector:_cmd];
  return nil;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"%@ %p (%@)",
          [self class], self, [self.mutableRequest URL]];
}

#if !GTM_IPHONE
- (void)finalize {
  [self stopFetchReleasingCallbacks:YES]; // releases connection_, destroys timers
  [super finalize];
}
#endif

- (void)dealloc {
#if DEBUG
  NSAssert(!isStopNotificationNeeded_,
           @"unbalanced fetcher notification for %@", [request_ URL]);
#endif

  // Note: if a connection or a retry timer was pending, then this instance
  // would be retained by those so it wouldn't be getting dealloc'd,
  // hence we don't need to stopFetch here
  [request_ release];
  [connection_ release];
  [downloadedData_ release];
  [downloadPath_ release];
  [temporaryDownloadPath_ release];
  [downloadFileHandle_ release];
  [credential_ release];
  [proxyCredential_ release];
  [postData_ release];
  [postStream_ release];
  [loggedStreamData_ release];
  [response_ release];
#if NS_BLOCKS_AVAILABLE
  [completionBlock_ release];
  [receivedDataBlock_ release];
  [sentDataBlock_ release];
  [retryBlock_ release];
#endif
  [userData_ release];
  [properties_ release];
  [delegateQueue_ release];
  [runLoopModes_ release];
  [fetchHistory_ release];
  [cookieStorage_ release];
  [authorizer_ release];
  [service_ release];
  [serviceHost_ release];
  [thread_ release];
  [retryTimer_ release];
  [comment_ release];
  [log_ release];
#if !STRIP_GTM_FETCH_LOGGING
  [logRequestBody_ release];
  [logResponseBody_ release];
#endif

  [super dealloc];
}

#pragma mark -

// Begin fetching the URL (or begin a retry fetch).  The delegate is retained
// for the duration of the fetch connection.

- (BOOL)beginFetchWithDelegate:(id)delegate
             didFinishSelector:(SEL)finishedSelector {
  GTMAssertSelectorNilOrImplementedWithArgs(delegate, finishedSelector, @encode(GTMHTTPFetcher *), @encode(NSData *), @encode(NSError *), 0);
  GTMAssertSelectorNilOrImplementedWithArgs(delegate, receivedDataSel_, @encode(GTMHTTPFetcher *), @encode(NSData *), 0);
  GTMAssertSelectorNilOrImplementedWithArgs(delegate, retrySel_, @encode(GTMHTTPFetcher *), @encode(BOOL), @encode(NSError *), 0);

  // We'll retain the delegate only during the outstanding connection (similar
  // to what Cocoa does with performSelectorOnMainThread:) and during
  // authorization or delays, since the app would crash
  // if the delegate was released before the fetch calls back
  [self setDelegate:delegate];
  finishedSel_ = finishedSelector;

  return [self beginFetchMayDelay:YES
                     mayAuthorize:YES];
}

- (BOOL)beginFetchMayDelay:(BOOL)mayDelay
              mayAuthorize:(BOOL)mayAuthorize {
  // This is the internal entry point for re-starting fetches
  NSError *error = nil;

  if (connection_ != nil) {
    NSAssert1(connection_ != nil, @"fetch object %@ being reused; this should never happen", self);
    goto CannotBeginFetch;
  }

  if (request_ == nil || [request_ URL] == nil) {
    NSAssert(request_ != nil, @"beginFetchWithDelegate requires a request with a URL");
    goto CannotBeginFetch;
  }

  self.downloadedData = nil;
  downloadedLength_ = 0;

  if (mayDelay && service_) {
    BOOL shouldFetchNow = [service_ fetcherShouldBeginFetching:self];
    if (!shouldFetchNow) {
      // the fetch is deferred, but will happen later
      return YES;
    }
  }

  NSString *effectiveHTTPMethod = [request_ valueForHTTPHeaderField:@"X-HTTP-Method-Override"];
  if (effectiveHTTPMethod == nil) {
    effectiveHTTPMethod = [request_ HTTPMethod];
  }
  BOOL isEffectiveHTTPGet = (effectiveHTTPMethod == nil
                             || [effectiveHTTPMethod isEqual:@"GET"]);

  if (postData_ || postStream_) {
    if (isEffectiveHTTPGet) {
      [request_ setHTTPMethod:@"POST"];
      isEffectiveHTTPGet = NO;
    }

    if (postData_) {
      [request_ setHTTPBody:postData_];
    } else {
      if ([self respondsToSelector:@selector(setupStreamLogging)]) {
        [self performSelector:@selector(setupStreamLogging)];
      }

      [request_ setHTTPBodyStream:postStream_];
    }
  }

  // We authorize after setting up the http method and body in the request
  // because OAuth 1 may need to sign the request body
  if (mayAuthorize && authorizer_) {
    BOOL isAuthorized = [authorizer_ isAuthorizedRequest:request_];
    if (!isAuthorized) {
      // authorization needed
      return [self authorizeRequest];
    }
  }

  [fetchHistory_ updateRequest:request_ isHTTPGet:isEffectiveHTTPGet];

  // set the default upload or download retry interval, if necessary
  if (isRetryEnabled_
      && maxRetryInterval_ <= kUnsetMaxRetryInterval) {
    if (isEffectiveHTTPGet || [effectiveHTTPMethod isEqual:@"HEAD"]) {
      [self setMaxRetryInterval:kDefaultMaxDownloadRetryInterval];
    } else {
      [self setMaxRetryInterval:kDefaultMaxUploadRetryInterval];
    }
  }

  [self addCookiesToRequest:request_];

  if (downloadPath_ != nil) {
    // downloading to a path, so create a temporary file and a file handle for
    // downloading
    NSString *tempPath = [self createTempDownloadFilePathForPath:downloadPath_];

    BOOL didCreate = [[NSData data] writeToFile:tempPath
                                        options:0
                                          error:&error];
    if (!didCreate) goto CannotBeginFetch;

    [self setTemporaryDownloadPath:tempPath];

    NSFileHandle *fh = [NSFileHandle fileHandleForWritingAtPath:tempPath];
    if (fh == nil) goto CannotBeginFetch;

    [self setDownloadFileHandle:fh];
  }

  // finally, start the connection

  Class connectionClass = [[self class] connectionClass];

  NSOperationQueue *delegateQueue = delegateQueue_;
  if (delegateQueue &&
      ![connectionClass instancesRespondToSelector:@selector(setDelegateQueue:)]) {
    // NSURLConnection has no setDelegateQueue: on iOS 4 and Mac OS X 10.5.
    delegateQueue = nil;
    self.delegateQueue = nil;
  }

#if DEBUG && TARGET_OS_IPHONE
  BOOL isPreIOS6 = (NSFoundationVersionNumber <= 890.1);
  if (isPreIOS6 && delegateQueue) {
    NSLog(@"GTMHTTPFetcher delegateQueue not safe in iOS 5");
  }
#endif

  if ([runLoopModes_ count] == 0 && delegateQueue == nil) {
    // No custom callback modes or queue were specified, so start the connection
    // on the current run loop in the current mode
    connection_ = [[connectionClass connectionWithRequest:request_
                                                 delegate:self] retain];
  } else {
    // Specify callbacks be on an operation queue or on the current run loop
    // in the specified modes
    connection_ = [[connectionClass alloc] initWithRequest:request_
                                                  delegate:self
                                          startImmediately:NO];
    if (delegateQueue) {
      [connection_ performSelector:@selector(setDelegateQueue:)
                        withObject:delegateQueue];
    } else if (runLoopModes_) {
      NSRunLoop *rl = [NSRunLoop currentRunLoop];
      for (NSString *mode in runLoopModes_) {
        [connection_ scheduleInRunLoop:rl forMode:mode];
      }
    }
    [connection_ start];
  }
  hasConnectionEnded_ = NO;

  if (!connection_) {
    NSAssert(connection_ != nil, @"beginFetchWithDelegate could not create a connection");
    goto CannotBeginFetch;
  }

  if (downloadFileHandle_ != nil) {
    // downloading to a file, so downloadedData_ remains nil
  } else {
    self.downloadedData = [NSMutableData data];
  }

#if GTM_BACKGROUND_FETCHING
  backgroundTaskIdentifer_ = 0;  // UIBackgroundTaskInvalid is 0 on iOS 4
  if (shouldFetchInBackground_) {
    // For iOS 3 compatibility, ensure that UIApp supports backgrounding
    UIApplication *app = [UIApplication sharedApplication];
    if ([app respondsToSelector:@selector(beginBackgroundTaskWithExpirationHandler:)]) {
      // Tell UIApplication that we want to continue even when the app is in the
      // background.
      NSThread *thread = [NSThread currentThread];
      backgroundTaskIdentifer_ = [app beginBackgroundTaskWithExpirationHandler:^{
        // Callback - this block is always invoked by UIApplication on the main
        // thread, but we want to run the user's callbacks on the thread used
        // to start the fetch.
        [self performSelector:@selector(backgroundFetchExpired)
                     onThread:thread
                   withObject:nil
                waitUntilDone:YES];
      }];
    }
  }
#endif

  // Once connection_ is non-nil we can send the start notification
  isStopNotificationNeeded_ = YES;
  NSNotificationCenter *defaultNC = [NSNotificationCenter defaultCenter];
  [defaultNC postNotificationName:kGTMHTTPFetcherStartedNotification
                           object:self];
  return YES;

CannotBeginFetch:
  [self failToBeginFetchDeferWithError:error];
  return NO;
}

- (void)failToBeginFetchDeferWithError:(NSError *)error {
  if (delegateQueue_) {
    // Deferring will happen by the callback being invoked on the specified
    // queue.
    [self failToBeginFetchWithError:error];
  } else {
    // No delegate queue has been specified, so put the callback
    // on an appropriate run loop.
    NSArray *modes = (runLoopModes_ ? runLoopModes_ :
                      [NSArray arrayWithObject:NSRunLoopCommonModes]);
    [self performSelector:@selector(failToBeginFetchWithError:)
                 onThread:[NSThread currentThread]
               withObject:error
            waitUntilDone:NO
                    modes:modes];
  }
}

- (void)failToBeginFetchWithError:(NSError *)error {
  if (error == nil) {
    error = [NSError errorWithDomain:kGTMHTTPFetcherErrorDomain
                                code:kGTMHTTPFetcherErrorDownloadFailed
                            userInfo:nil];
  }

  [[self retain] autorelease];  // In case the callback releases us

  [self invokeFetchCallbacksOnDelegateQueueWithData:nil
                                              error:error];

  [self releaseCallbacks];

  [service_ fetcherDidStop:self];

  self.authorizer = nil;

  if (temporaryDownloadPath_) {
    [[NSFileManager defaultManager] removeItemAtPath:temporaryDownloadPath_
                                               error:NULL];
    self.temporaryDownloadPath = nil;
  }
}

#if GTM_BACKGROUND_FETCHING
- (void)backgroundFetchExpired {
  // On background expiration, we stop the fetch and invoke the callbacks
  NSError *error = [NSError errorWithDomain:kGTMHTTPFetcherErrorDomain
                                       code:kGTMHTTPFetcherErrorBackgroundExpiration
                                   userInfo:nil];
  [self invokeFetchCallbacksOnDelegateQueueWithData:nil
                                              error:error];
  @synchronized(self) {
    // Stopping the fetch here will indirectly call endBackgroundTask
    [self stopFetchReleasingCallbacks:NO];

    [self releaseCallbacks];
    self.authorizer = nil;
  }
}

- (void)endBackgroundTask {
  @synchronized(self) {
    // Whenever the connection stops or background execution expires,
    // we need to tell UIApplication we're done
    if (backgroundTaskIdentifer_) {
      // If backgroundTaskIdentifer_ is non-zero, we know we're on iOS 4
      UIApplication *app = [UIApplication sharedApplication];
      [app endBackgroundTask:backgroundTaskIdentifer_];

      backgroundTaskIdentifer_ = 0;
    }
  }
}
#endif // GTM_BACKGROUND_FETCHING

- (BOOL)authorizeRequest {
  id authorizer = self.authorizer;
  SEL asyncAuthSel = @selector(authorizeRequest:delegate:didFinishSelector:);
  if ([authorizer respondsToSelector:asyncAuthSel]) {
    SEL callbackSel = @selector(authorizer:request:finishedWithError:);
    [authorizer authorizeRequest:request_
                        delegate:self
               didFinishSelector:callbackSel];
    return YES;
  } else {
    NSAssert(authorizer == nil, @"invalid authorizer for fetch");

    // No authorizing possible, and authorizing happens only after any delay;
    // just begin fetching
    return [self beginFetchMayDelay:NO
                       mayAuthorize:NO];
  }
}

- (void)authorizer:(id <GTMFetcherAuthorizationProtocol>)auth
           request:(NSMutableURLRequest *)request
 finishedWithError:(NSError *)error {
  if (error != nil) {
    // We can't fetch without authorization
    [self failToBeginFetchDeferWithError:error];
  } else {
    [self beginFetchMayDelay:NO
                mayAuthorize:NO];
  }
}

#if NS_BLOCKS_AVAILABLE
- (BOOL)beginFetchWithCompletionHandler:(void (^)(NSData *data, NSError *error))handler {
  self.completionBlock = handler;

  // The user may have called setDelegate: earlier if they want to use other
  // delegate-style callbacks during the fetch; otherwise, the delegate is nil,
  // which is fine.
  return [self beginFetchWithDelegate:[self delegate]
                    didFinishSelector:nil];
}
#endif

- (NSString *)createTempDownloadFilePathForPath:(NSString *)targetPath {
  NSString *tempDir = nil;

#if (!TARGET_OS_IPHONE && (MAC_OS_X_VERSION_MAX_ALLOWED >= 1060))
  // Find an appropriate directory for the download, ideally on the same disk
  // as the final target location so the temporary file won't have to be moved
  // to a different disk.
  //
  // Available in SDKs for 10.6 and iOS 4
  //
  // Oct 2011: We previously also used URLForDirectory for
  //   (TARGET_OS_IPHONE && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 40000))
  // but that is returning a non-temporary directory for iOS, unfortunately

  SEL sel = @selector(URLForDirectory:inDomain:appropriateForURL:create:error:);
  if ([NSFileManager instancesRespondToSelector:sel]) {
    NSError *error = nil;
    NSURL *targetURL = [NSURL fileURLWithPath:targetPath];
    NSFileManager *fileMgr = [NSFileManager defaultManager];

    NSURL *tempDirURL = [fileMgr URLForDirectory:NSItemReplacementDirectory
                                        inDomain:NSUserDomainMask
                               appropriateForURL:targetURL
                                          create:YES
                                           error:&error];
    tempDir = [tempDirURL path];
  }
#endif

  if (tempDir == nil) {
    tempDir = NSTemporaryDirectory();
  }

  static unsigned int counter = 0;
  NSString *name = [NSString stringWithFormat:@"gtmhttpfetcher_%u_%u",
                        ++counter, (unsigned int) arc4random()];
  NSString *result = [tempDir stringByAppendingPathComponent:name];
  return result;
}

- (void)addCookiesToRequest:(NSMutableURLRequest *)request {
  // Get cookies for this URL from our storage array, if
  // we have a storage array
  if (cookieStorageMethod_ != kGTMHTTPFetcherCookieStorageMethodSystemDefault
      && cookieStorageMethod_ != kGTMHTTPFetcherCookieStorageMethodNone) {

    NSArray *cookies = [cookieStorage_ cookiesForURL:[request URL]];
    if ([cookies count] > 0) {

      NSDictionary *headerFields = [NSHTTPCookie requestHeaderFieldsWithCookies:cookies];
      NSString *cookieHeader = [headerFields objectForKey:@"Cookie"]; // key used in header dictionary
      if (cookieHeader) {
        [request addValue:cookieHeader forHTTPHeaderField:@"Cookie"]; // header name
      }
    }
  }
}

// Returns YES if this is in the process of fetching a URL, or waiting to
// retry, or waiting for authorization, or waiting to be issued by the
// service object
- (BOOL)isFetching {
  if (connection_ != nil || retryTimer_ != nil) return YES;

  BOOL isAuthorizing = [authorizer_ isAuthorizingRequest:request_];
  if (isAuthorizing) return YES;

  BOOL isDelayed = [service_ isDelayingFetcher:self];
  return isDelayed;
}

// Returns the status code set in connection:didReceiveResponse:
- (NSInteger)statusCode {

  NSInteger statusCode;

  if (response_ != nil
    && [response_ respondsToSelector:@selector(statusCode)]) {

    statusCode = [(NSHTTPURLResponse *)response_ statusCode];
  } else {
    //  Default to zero, in hopes of hinting "Unknown" (we can't be
    //  sure that things are OK enough to use 200).
    statusCode = 0;
  }
  return statusCode;
}

- (NSDictionary *)responseHeaders {
  if (response_ != nil
      && [response_ respondsToSelector:@selector(allHeaderFields)]) {

    NSDictionary *headers = [(NSHTTPURLResponse *)response_ allHeaderFields];
    return headers;
  }
  return nil;
}

- (void)releaseCallbacks {
  [delegate_ autorelease];
  delegate_ = nil;

  [delegateQueue_ autorelease];
  delegateQueue_ = nil;

#if NS_BLOCKS_AVAILABLE
  self.completionBlock = nil;
  self.sentDataBlock = nil;
  self.receivedDataBlock = nil;
  self.retryBlock = nil;
#endif
}

// Cancel the fetch of the URL that's currently in progress.
- (void)stopFetchReleasingCallbacks:(BOOL)shouldReleaseCallbacks {
  id <GTMHTTPFetcherServiceProtocol> service;

  // if the connection or the retry timer is all that's retaining the fetcher,
  // we want to be sure this instance survives stopping at least long enough for
  // the stack to unwind
  [[self retain] autorelease];

  [self destroyRetryTimer];

  @synchronized(self) {
    service = [[service_ retain] autorelease];

    if (connection_) {
      // in case cancelling the connection calls this recursively, we want
      // to ensure that we'll only release the connection and delegate once,
      // so first set connection_ to nil
      NSURLConnection* oldConnection = connection_;
      connection_ = nil;

      if (!hasConnectionEnded_) {
        [oldConnection cancel];
      }

      // this may be called in a callback from the connection, so use autorelease
      [oldConnection autorelease];
    }
  }

  // send the stopped notification
  [self sendStopNotificationIfNeeded];

  @synchronized(self) {
    [authorizer_ stopAuthorizationForRequest:request_];

    if (shouldReleaseCallbacks) {
      [self releaseCallbacks];

      self.authorizer = nil;
    }

    if (temporaryDownloadPath_) {
      [[NSFileManager defaultManager] removeItemAtPath:temporaryDownloadPath_
                                                 error:NULL];
      self.temporaryDownloadPath = nil;
    }
  }

  [service fetcherDidStop:self];

#if GTM_BACKGROUND_FETCHING
  [self endBackgroundTask];
#endif
}

// External stop method
- (void)stopFetching {
  [self stopFetchReleasingCallbacks:YES];
}

- (void)sendStopNotificationIfNeeded {
  BOOL sendNow = NO;
  @synchronized(self) {
    if (isStopNotificationNeeded_) {
      isStopNotificationNeeded_ = NO;
      sendNow = YES;
    }
  }

  if (sendNow) {
    NSNotificationCenter *defaultNC = [NSNotificationCenter defaultCenter];
    [defaultNC postNotificationName:kGTMHTTPFetcherStoppedNotification
                             object:self];
  }
}

- (void)retryFetch {
  [self stopFetchReleasingCallbacks:NO];

  [self beginFetchWithDelegate:delegate_
             didFinishSelector:finishedSel_];
}

- (void)waitForCompletionWithTimeout:(NSTimeInterval)timeoutInSeconds {
  NSDate* giveUpDate = [NSDate dateWithTimeIntervalSinceNow:timeoutInSeconds];

  // Loop until the callbacks have been called and released, and until
  // the connection is no longer pending, or until the timeout has expired
  BOOL isMainThread = [NSThread isMainThread];

  while ((!hasConnectionEnded_
#if NS_BLOCKS_AVAILABLE
          || completionBlock_ != nil
#endif
          || delegate_ != nil)
         && [giveUpDate timeIntervalSinceNow] > 0) {

    // Run the current run loop 1/1000 of a second to give the networking
    // code a chance to work
    if (isMainThread || delegateQueue_ == nil) {
      NSDate *stopDate = [NSDate dateWithTimeIntervalSinceNow:0.001];
      [[NSRunLoop currentRunLoop] runUntilDate:stopDate];
    } else {
      [NSThread sleepForTimeInterval:0.001];
    }
  }
}

#pragma mark NSURLConnection Delegate Methods

//
// NSURLConnection Delegate Methods
//

// This method just says "follow all redirects", which _should_ be the default behavior,
// According to file:///Developer/ADC%20Reference%20Library/documentation/Cocoa/Conceptual/URLLoadingSystem
// but the redirects were not being followed until I added this method.  May be
// a bug in the NSURLConnection code, or the documentation.
//
// In OS X 10.4.8 and earlier, the redirect request doesn't
// get the original's headers and body. This causes POSTs to fail.
// So we construct a new request, a copy of the original, with overrides from the
// redirect.
//
// Docs say that if redirectResponse is nil, just return the redirectRequest.

- (NSURLRequest *)connection:(NSURLConnection *)connection
             willSendRequest:(NSURLRequest *)redirectRequest
            redirectResponse:(NSURLResponse *)redirectResponse {
  @synchronized(self) {
    if (redirectRequest && redirectResponse) {
      // save cookies from the response
      [self handleCookiesForResponse:redirectResponse];

      NSMutableURLRequest *newRequest = [[request_ mutableCopy] autorelease];
      // copy the URL
      NSURL *redirectURL = [redirectRequest URL];
      NSURL *url = [newRequest URL];

      // disallow scheme changes (say, from https to http)
      NSString *redirectScheme = [url scheme];
      NSString *newScheme = [redirectURL scheme];
      NSString *newResourceSpecifier = [redirectURL resourceSpecifier];

      if ([redirectScheme caseInsensitiveCompare:@"http"] == NSOrderedSame
          && newScheme != nil
          && [newScheme caseInsensitiveCompare:@"https"] == NSOrderedSame) {

        // allow the change from http to https
        redirectScheme = newScheme;
      }

      NSString *newUrlString = [NSString stringWithFormat:@"%@:%@",
        redirectScheme, newResourceSpecifier];

      NSURL *newURL = [NSURL URLWithString:newUrlString];
      [newRequest setURL:newURL];

      // any headers in the redirect override headers in the original.
      NSDictionary *redirectHeaders = [redirectRequest allHTTPHeaderFields];
      for (NSString *key in redirectHeaders) {
        NSString *value = [redirectHeaders objectForKey:key];
        [newRequest setValue:value forHTTPHeaderField:key];
      }

      [self addCookiesToRequest:newRequest];

      redirectRequest = newRequest;

      // log the response we just received
      [self setResponse:redirectResponse];
      [self logNowWithError:nil];

      // update the request for future logging
      NSMutableURLRequest *mutable = [[redirectRequest mutableCopy] autorelease];
      [self setMutableRequest:mutable];
    }
    return redirectRequest;
  }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
  @synchronized(self) {
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse
    // it can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
    [downloadedData_ setLength:0];
    [downloadFileHandle_ truncateFileAtOffset:0];
    downloadedLength_ = 0;

    [self setResponse:response];

    // Save cookies from the response
    [self handleCookiesForResponse:response];
  }
}


// handleCookiesForResponse: handles storage of cookies for responses passed to
// connection:willSendRequest:redirectResponse: and connection:didReceiveResponse:
- (void)handleCookiesForResponse:(NSURLResponse *)response {

  if (cookieStorageMethod_ == kGTMHTTPFetcherCookieStorageMethodSystemDefault
    || cookieStorageMethod_ == kGTMHTTPFetcherCookieStorageMethodNone) {

    // do nothing special for NSURLConnection's default storage mechanism
    // or when we're ignoring cookies

  } else if ([response respondsToSelector:@selector(allHeaderFields)]) {

    // grab the cookies from the header as NSHTTPCookies and store them either
    // into our static array or into the fetchHistory

    NSDictionary *responseHeaderFields = [(NSHTTPURLResponse *)response allHeaderFields];
    if (responseHeaderFields) {

      NSArray *cookies = [NSHTTPCookie cookiesWithResponseHeaderFields:responseHeaderFields
                                                                forURL:[response URL]];
      if ([cookies count] > 0) {
        [cookieStorage_ setCookies:cookies];
      }
    }
  }
}

-(void)connection:(NSURLConnection *)connection
didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
  @synchronized(self) {
    if ([challenge previousFailureCount] <= 2) {

      NSURLCredential *credential = credential_;

      if ([[challenge protectionSpace] isProxy] && proxyCredential_ != nil) {
        credential = proxyCredential_;
      }

      // Here, if credential is still nil, then we *could* try to get it from
      // NSURLCredentialStorage's defaultCredentialForProtectionSpace:.
      // We don't, because we're assuming:
      //
      // - for server credentials, we only want ones supplied by the program
      //   calling http fetcher
      // - for proxy credentials, if one were necessary and available in the
      //   keychain, it would've been found automatically by NSURLConnection
      //   and this challenge delegate method never would've been called
      //   anyway

      if (credential) {
        // try the credential
        [[challenge sender] useCredential:credential
               forAuthenticationChallenge:challenge];
        return;
      }
    }

    // If we don't have credentials, or we've already failed auth 3x,
    // report the error, putting the challenge as a value in the userInfo
    // dictionary.
#if DEBUG
    NSAssert(!isCancellingChallenge_, @"isCancellingChallenge_ unexpected");
#endif
    NSDictionary *userInfo = [NSDictionary dictionaryWithObject:challenge
                                                         forKey:kGTMHTTPFetcherErrorChallengeKey];
    NSError *error = [NSError errorWithDomain:kGTMHTTPFetcherErrorDomain
                                         code:kGTMHTTPFetcherErrorAuthenticationChallengeFailed
                                     userInfo:userInfo];

    // cancelAuthenticationChallenge seems to indirectly call
    // connection:didFailWithError: now, though that isn't documented
    //
    // We'll use an ivar to make the indirect invocation of the
    // delegate method do nothing.
    isCancellingChallenge_ = YES;
    [[challenge sender] cancelAuthenticationChallenge:challenge];
    isCancellingChallenge_ = NO;

    [self connection:connection didFailWithError:error];
  }
}

- (void)invokeFetchCallbacksWithData:(NSData *)data
                               error:(NSError *)error {
  // To avoid deadlocks, this should not be called inside of @synchronized(self)
  id target;
  SEL sel;
#if NS_BLOCKS_AVAILABLE
  void (^block)(NSData *, NSError *);
#endif
  @synchronized(self) {
    target = delegate_;
    sel = finishedSel_;
    block = completionBlock_;
  }

  [[self retain] autorelease];  // In case the callback releases us

  [self invokeFetchCallback:sel
                     target:target
                       data:data
                      error:error];

#if NS_BLOCKS_AVAILABLE
  if (block) {
    block(data, error);
  }
#endif
}

- (void)invokeFetchCallback:(SEL)sel
                     target:(id)target
                       data:(NSData *)data
                      error:(NSError *)error {
  // This method is available to subclasses which may provide a customized
  // target pointer.
  if (target && sel) {
    NSMethodSignature *sig = [target methodSignatureForSelector:sel];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:sig];
    [invocation setSelector:sel];
    [invocation setTarget:target];
    [invocation setArgument:&self atIndex:2];
    [invocation setArgument:&data atIndex:3];
    [invocation setArgument:&error atIndex:4];
    [invocation invoke];
  }
}

- (void)invokeFetchCallbacksOnDelegateQueueWithData:(NSData *)data
                                              error:(NSError *)error {
  // This is called by methods that are not already on the delegateQueue
  // (as NSURLConnection callbacks should already be, but other failures
  // are not.)
  if (!delegateQueue_) {
    [self invokeFetchCallbacksWithData:data error:error];
  }

  // Values may be nil.
  NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:2];
  [dict setValue:data forKey:kCallbackData];
  [dict setValue:error forKey:kCallbackError];
  NSInvocationOperation *op =
    [[[NSInvocationOperation alloc] initWithTarget:self
                                          selector:@selector(invokeOnQueueWithDictionary:)
                                            object:dict] autorelease];
  [delegateQueue_ addOperation:op];
}

- (void)invokeOnQueueWithDictionary:(NSDictionary *)dict {
  NSData *data = [dict objectForKey:kCallbackData];
  NSError *error = [dict objectForKey:kCallbackError];

  [self invokeFetchCallbacksWithData:data error:error];
}


- (void)invokeSentDataCallback:(SEL)sel
                        target:(id)target
               didSendBodyData:(NSInteger)bytesWritten
             totalBytesWritten:(NSInteger)totalBytesWritten
     totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
  if (target && sel) {
    NSMethodSignature *sig = [target methodSignatureForSelector:sel];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:sig];
    [invocation setSelector:sel];
    [invocation setTarget:target];
    [invocation setArgument:&self atIndex:2];
    [invocation setArgument:&bytesWritten atIndex:3];
    [invocation setArgument:&totalBytesWritten atIndex:4];
    [invocation setArgument:&totalBytesExpectedToWrite atIndex:5];
    [invocation invoke];
  }
}

- (BOOL)invokeRetryCallback:(SEL)sel
                     target:(id)target
                  willRetry:(BOOL)willRetry
                      error:(NSError *)error {
  if (target && sel) {
    NSMethodSignature *sig = [target methodSignatureForSelector:sel];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:sig];
    [invocation setSelector:sel];
    [invocation setTarget:target];
    [invocation setArgument:&self atIndex:2];
    [invocation setArgument:&willRetry atIndex:3];
    [invocation setArgument:&error atIndex:4];
    [invocation invoke];

    [invocation getReturnValue:&willRetry];
  }
  return willRetry;
}

- (void)connection:(NSURLConnection *)connection
   didSendBodyData:(NSInteger)bytesWritten
 totalBytesWritten:(NSInteger)totalBytesWritten
totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
  @synchronized(self) {
    SEL sel = [self sentDataSelector];
    [self invokeSentDataCallback:sel
                          target:delegate_
                 didSendBodyData:bytesWritten
               totalBytesWritten:totalBytesWritten
       totalBytesExpectedToWrite:totalBytesExpectedToWrite];

#if NS_BLOCKS_AVAILABLE
    if (sentDataBlock_) {
      sentDataBlock_(bytesWritten, totalBytesWritten, totalBytesExpectedToWrite);
    }
#endif
  }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
  @synchronized(self) {
#if DEBUG
    // The download file handle should be set before the fetch is started, not
    // after
    NSAssert((downloadFileHandle_ == nil) != (downloadedData_ == nil),
             @"received data accumulates as NSData or NSFileHandle, not both");
#endif

    if (downloadFileHandle_ != nil) {
      // Append to file
      @try {
        [downloadFileHandle_ writeData:data];

        downloadedLength_ = [downloadFileHandle_ offsetInFile];
      }
      @catch (NSException *exc) {
        // Couldn't write to file, probably due to a full disk
        NSDictionary *userInfo = [NSDictionary dictionaryWithObject:[exc reason]
                                                             forKey:NSLocalizedDescriptionKey];
        NSError *error = [NSError errorWithDomain:kGTMHTTPFetcherStatusDomain
                                             code:kGTMHTTPFetcherErrorFileHandleException
                                         userInfo:userInfo];
        [self connection:connection didFailWithError:error];
        return;
      }
    } else {
      // append to mutable data
      [downloadedData_ appendData:data];

      downloadedLength_ = [downloadedData_ length];
    }

    if (receivedDataSel_) {
      [delegate_ performSelector:receivedDataSel_
                      withObject:self
                      withObject:downloadedData_];
    }

#if NS_BLOCKS_AVAILABLE
    if (receivedDataBlock_) {
      receivedDataBlock_(downloadedData_);
    }
#endif
  }
}

// For error 304's ("Not Modified") where we've cached the data, return
// status 200 ("OK") to the caller (but leave the fetcher status as 304)
// and copy the cached data.
//
// For other errors or if there's no cached data, just return the actual status.
- (NSData *)cachedDataForStatus {
  if ([self statusCode] == kGTMHTTPFetcherStatusNotModified
      && [fetchHistory_ shouldCacheETaggedData]) {
    NSData *cachedData = [fetchHistory_ cachedDataForRequest:request_];
    return cachedData;
  }
  return nil;
}

- (NSInteger)statusAfterHandlingNotModifiedError {
  NSInteger status = [self statusCode];
  NSData *cachedData = [self cachedDataForStatus];
  if (cachedData) {
    // Forge the status to pass on to the delegate
    status = 200;

    // Copy our stored data
    if (downloadFileHandle_ != nil) {
      @try {
        // Downloading to a file handle won't save to the cache (the data is
        // likely inappropriately large for caching), but will still read from
        // the cache, on the unlikely chance that the response was Not Modified
        // and the URL response was indeed present in the cache.
        [downloadFileHandle_ truncateFileAtOffset:0];
        [downloadFileHandle_ writeData:cachedData];
        downloadedLength_ = [downloadFileHandle_ offsetInFile];
      }
      @catch (NSException *) {
        // Failed to write data, likely due to lack of disk space
        status = kGTMHTTPFetcherErrorFileHandleException;
      }
    } else {
      [downloadedData_ setData:cachedData];
      downloadedLength_ = [cachedData length];
    }
  }
  return status;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
  BOOL shouldStopFetching = YES;
  BOOL shouldSendStopNotification = NO;
  NSError *error = nil;
  NSData *downloadedData;
#if !STRIP_GTM_FETCH_LOGGING
  BOOL shouldDeferLogging = NO;
#endif
  BOOL shouldBeginRetryTimer = NO;
  BOOL hasLogged = NO;

  @synchronized(self) {
    // We no longer need to cancel the connection
    hasConnectionEnded_ = YES;

    // Skip caching ETagged results when the data is being saved to a file
    if (downloadFileHandle_ == nil) {
      [fetchHistory_ updateFetchHistoryWithRequest:request_
                                          response:response_
                                    downloadedData:downloadedData_];
    } else {
      [fetchHistory_ removeCachedDataForRequest:request_];
    }

    [[self retain] autorelease]; // in case the callback releases us

    NSInteger status = [self statusCode];
    if ([self cachedDataForStatus] != nil) {
      // Log the pre-cache response.
      [self logNowWithError:nil];
      hasLogged = YES;
      status = [self statusAfterHandlingNotModifiedError];
    }

    shouldSendStopNotification = YES;

    if (status >= 0 && status < 300) {
      // success
      if (downloadPath_) {
        // Avoid deleting the downloaded file when the fetch stops
        [downloadFileHandle_ closeFile];
        self.downloadFileHandle = nil;

        NSFileManager *fileMgr = [NSFileManager defaultManager];
        [fileMgr removeItemAtPath:downloadPath_
                            error:NULL];

        if ([fileMgr moveItemAtPath:temporaryDownloadPath_
                             toPath:downloadPath_
                              error:&error]) {
          self.temporaryDownloadPath = nil;
        }
      }
    } else {
      // unsuccessful
      if (!hasLogged) {
        [self logNowWithError:nil];
        hasLogged = YES;
      }
      // Status over 300; retry or notify the delegate of failure
      if ([self shouldRetryNowForStatus:status error:nil]) {
        // retrying
        shouldBeginRetryTimer = YES;
        shouldStopFetching = NO;
      } else {
        NSDictionary *userInfo = nil;
        if ([downloadedData_ length] > 0) {
          userInfo = [NSDictionary dictionaryWithObject:downloadedData_
                                                 forKey:kGTMHTTPFetcherStatusDataKey];
        }
        error = [NSError errorWithDomain:kGTMHTTPFetcherStatusDomain
                                    code:status
                                userInfo:userInfo];
      }
    }
    downloadedData = downloadedData_;
#if !STRIP_GTM_FETCH_LOGGING
    shouldDeferLogging = shouldDeferResponseBodyLogging_;
#endif
  }

  if (shouldBeginRetryTimer) {
    [self beginRetryTimer];
  }

  if (shouldSendStopNotification) {
    // We want to send the stop notification before calling the delegate's
    // callback selector, since the callback selector may release all of
    // the fetcher properties that the client is using to track the fetches.
    //
    // We'll also stop now so that, to any observers watching the notifications,
    // it doesn't look like our wait for a retry (which may be long,
    // 30 seconds or more) is part of the network activity.
    [self sendStopNotificationIfNeeded];
  }

  if (shouldStopFetching) {
    // Call the callbacks (outside of the @synchronized to avoid deadlocks.)
    [self invokeFetchCallbacksWithData:downloadedData
                                 error:error];
    BOOL shouldRelease = [self shouldReleaseCallbacksUponCompletion];
    [self stopFetchReleasingCallbacks:shouldRelease];
  }

  @synchronized(self) {
    BOOL shouldLogNow = !hasLogged;
#if !STRIP_GTM_FETCH_LOGGING
    if (shouldDeferLogging) shouldLogNow = NO;
#endif
    if (shouldLogNow) {
      [self logNowWithError:nil];
    }
  }
}

- (BOOL)shouldReleaseCallbacksUponCompletion {
  // A subclass can override this to keep callbacks around after the
  // connection has finished successfully
  return YES;
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
  @synchronized(self) {
    // Prevent the failure callback from being called twice, since the stopFetch
    // call below (either the explicit one at the end of this method, or the
    // implicit one when the retry occurs) will release the delegate.
    if (connection_ == nil) return;

    // If this method was invoked indirectly by cancellation of an authentication
    // challenge, defer this until it is called again with the proper error object
    if (isCancellingChallenge_) return;

    // We no longer need to cancel the connection
    hasConnectionEnded_ = YES;

    [self logNowWithError:error];
  }

  // See comment about sendStopNotificationIfNeeded
  // in connectionDidFinishLoading:
  [self sendStopNotificationIfNeeded];

  if ([self shouldRetryNowForStatus:0 error:error]) {
    [self beginRetryTimer];
  } else {
    [[self retain] autorelease]; // in case the callback releases us

    [self invokeFetchCallbacksWithData:nil
                                 error:error];

    [self stopFetchReleasingCallbacks:YES];
  }
}

- (void)logNowWithError:(NSError *)error {
  // If the logging category is available, then log the current request,
  // response, data, and error
  if ([self respondsToSelector:@selector(logFetchWithError:)]) {
    [self performSelector:@selector(logFetchWithError:) withObject:error];
  }
}

#pragma mark Retries

- (BOOL)isRetryError:(NSError *)error {

  struct retryRecord {
    NSString *const domain;
    int code;
  };

  struct retryRecord retries[] = {
    { kGTMHTTPFetcherStatusDomain, 408 }, // request timeout
    { kGTMHTTPFetcherStatusDomain, 503 }, // service unavailable
    { kGTMHTTPFetcherStatusDomain, 504 }, // request timeout
    { NSURLErrorDomain, NSURLErrorTimedOut },
    { NSURLErrorDomain, NSURLErrorNetworkConnectionLost },
    { nil, 0 }
  };

  // NSError's isEqual always returns false for equal but distinct instances
  // of NSError, so we have to compare the domain and code values explicitly

  for (int idx = 0; retries[idx].domain != nil; idx++) {

    if ([[error domain] isEqual:retries[idx].domain]
        && [error code] == retries[idx].code) {

      return YES;
    }
  }
  return NO;
}


// shouldRetryNowForStatus:error: returns YES if the user has enabled retries
// and the status or error is one that is suitable for retrying.  "Suitable"
// means either the isRetryError:'s list contains the status or error, or the
// user's retrySelector: is present and returns YES when called, or the
// authorizer may be able to fix.
- (BOOL)shouldRetryNowForStatus:(NSInteger)status
                          error:(NSError *)error {
  // Determine if a refreshed authorizer may avoid an authorization error
  BOOL shouldRetryForAuthRefresh = NO;
  BOOL isFirstAuthError = (authorizer_ != nil)
    && !hasAttemptedAuthRefresh_
    && (status == kGTMHTTPFetcherStatusUnauthorized); // 401

  if (isFirstAuthError) {
    if ([authorizer_ respondsToSelector:@selector(primeForRefresh)]) {
      BOOL hasPrimed = [authorizer_ primeForRefresh];
      if (hasPrimed) {
        shouldRetryForAuthRefresh = YES;
        hasAttemptedAuthRefresh_ = YES;
        [request_ setValue:nil forHTTPHeaderField:@"Authorization"];
      }
    }
  }

  // Determine if we're doing exponential backoff retries
  BOOL shouldDoIntervalRetry = [self isRetryEnabled]
    && ([self nextRetryInterval] < [self maxRetryInterval]);

  BOOL willRetry = NO;
  BOOL canRetry = shouldRetryForAuthRefresh || shouldDoIntervalRetry;
  if (canRetry) {
    // Check if this is a retryable error
    if (error == nil) {
      // Make an error for the status
      NSDictionary *userInfo = nil;
      if ([downloadedData_ length] > 0) {
        userInfo = [NSDictionary dictionaryWithObject:downloadedData_
                                               forKey:kGTMHTTPFetcherStatusDataKey];
      }
      error = [NSError errorWithDomain:kGTMHTTPFetcherStatusDomain
                                  code:status
                              userInfo:userInfo];
    }

    willRetry = shouldRetryForAuthRefresh || [self isRetryError:error];

    // If the user has installed a retry callback, consult that
    willRetry = [self invokeRetryCallback:retrySel_
                                   target:delegate_
                                willRetry:willRetry
                                    error:error];
#if NS_BLOCKS_AVAILABLE
    if (retryBlock_) {
      willRetry = retryBlock_(willRetry, error);
    }
#endif
  }
  return willRetry;
}

- (void)beginRetryTimer {
  @synchronized(self) {
    if (delegateQueue_ != nil && ![NSThread isMainThread]) {
      // A delegate queue is set, so the thread we're running on may not
      // have a run loop. We'll defer creating and starting the timer
      // until we're on the main thread to ensure it has a run loop.
      // (If we weren't supporting 10.5, we could use dispatch_after instead
      // of an NSTimer.)
      [self performSelectorOnMainThread:_cmd
                             withObject:nil
                          waitUntilDone:NO];
      return;
    }
  }

  NSTimeInterval nextInterval = [self nextRetryInterval];
  NSTimeInterval maxInterval = [self maxRetryInterval];
  NSTimeInterval newInterval = MIN(nextInterval, maxInterval);

  [self primeRetryTimerWithNewTimeInterval:newInterval];

  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc postNotificationName:kGTMHTTPFetcherRetryDelayStartedNotification
                    object:self];
}

- (void)primeRetryTimerWithNewTimeInterval:(NSTimeInterval)secs {

  [self destroyRetryTimer];

  @synchronized(self) {
    lastRetryInterval_ = secs;

    retryTimer_ = [NSTimer timerWithTimeInterval:secs
                                          target:self
                                        selector:@selector(retryTimerFired:)
                                        userInfo:nil
                                         repeats:NO];
    [retryTimer_ retain];

    NSRunLoop *timerRL = (self.delegateQueue ?
                          [NSRunLoop mainRunLoop] : [NSRunLoop currentRunLoop]);
    [timerRL addTimer:retryTimer_
              forMode:NSDefaultRunLoopMode];
  }
}

- (void)retryTimerFired:(NSTimer *)timer {
  [self destroyRetryTimer];

  @synchronized(self) {
    retryCount_++;

    [self retryFetch];
  }
}

- (void)destroyRetryTimer {
  BOOL shouldNotify = NO;
  @synchronized(self) {
    if (retryTimer_) {
      [retryTimer_ invalidate];
      [retryTimer_ autorelease];
      retryTimer_ = nil;
      shouldNotify = YES;
    }
  }

  if (shouldNotify) {
    NSNotificationCenter *defaultNC = [NSNotificationCenter defaultCenter];
    [defaultNC postNotificationName:kGTMHTTPFetcherRetryDelayStoppedNotification
                             object:self];
  }
}

- (NSUInteger)retryCount {
  return retryCount_;
}

- (NSTimeInterval)nextRetryInterval {
  // The next wait interval is the factor (2.0) times the last interval,
  // but never less than the minimum interval.
  NSTimeInterval secs = lastRetryInterval_ * retryFactor_;
  secs = MIN(secs, maxRetryInterval_);
  secs = MAX(secs, minRetryInterval_);

  return secs;
}

- (BOOL)isRetryEnabled {
  return isRetryEnabled_;
}

- (void)setRetryEnabled:(BOOL)flag {

  if (flag && !isRetryEnabled_) {
    // We defer initializing these until the user calls setRetryEnabled
    // to avoid using the random number generator if it's not needed.
    // However, this means min and max intervals for this fetcher are reset
    // as a side effect of calling setRetryEnabled.
    //
    // Make an initial retry interval random between 1.0 and 2.0 seconds
    [self setMinRetryInterval:0.0];
    [self setMaxRetryInterval:kUnsetMaxRetryInterval];
    [self setRetryFactor:2.0];
    lastRetryInterval_ = 0.0;
  }
  isRetryEnabled_ = flag;
};

- (NSTimeInterval)maxRetryInterval {
  return maxRetryInterval_;
}

- (void)setMaxRetryInterval:(NSTimeInterval)secs {
  if (secs > 0) {
    maxRetryInterval_ = secs;
  } else {
    maxRetryInterval_ = kUnsetMaxRetryInterval;
  }
}

- (double)minRetryInterval {
  return minRetryInterval_;
}

- (void)setMinRetryInterval:(NSTimeInterval)secs {
  if (secs > 0) {
    minRetryInterval_ = secs;
  } else {
    // Set min interval to a random value between 1.0 and 2.0 seconds
    // so that if multiple clients start retrying at the same time, they'll
    // repeat at different times and avoid overloading the server
    minRetryInterval_ = 1.0 + ((double)(arc4random() & 0x0FFFF) / (double) 0x0FFFF);
  }
}

#pragma mark Getters and Setters

@dynamic cookieStorageMethod,
         retryEnabled,
         maxRetryInterval,
         minRetryInterval,
         retryCount,
         nextRetryInterval,
         statusCode,
         responseHeaders,
         fetchHistory,
         userData,
         properties;

@synthesize mutableRequest = request_,
            credential = credential_,
            proxyCredential = proxyCredential_,
            postData = postData_,
            postStream = postStream_,
            delegate = delegate_,
            authorizer = authorizer_,
            service = service_,
            serviceHost = serviceHost_,
            servicePriority = servicePriority_,
            thread = thread_,
            sentDataSelector = sentDataSel_,
            receivedDataSelector = receivedDataSel_,
            retrySelector = retrySel_,
            retryFactor = retryFactor_,
            response = response_,
            downloadedLength = downloadedLength_,
            downloadedData = downloadedData_,
            downloadPath = downloadPath_,
            temporaryDownloadPath = temporaryDownloadPath_,
            downloadFileHandle = downloadFileHandle_,
            delegateQueue = delegateQueue_,
            runLoopModes = runLoopModes_,
            comment = comment_,
            log = log_,
            cookieStorage = cookieStorage_;

#if NS_BLOCKS_AVAILABLE
@synthesize completionBlock = completionBlock_,
            sentDataBlock = sentDataBlock_,
            receivedDataBlock = receivedDataBlock_,
            retryBlock = retryBlock_;
#endif

@synthesize shouldFetchInBackground = shouldFetchInBackground_;

- (NSInteger)cookieStorageMethod {
  return cookieStorageMethod_;
}

- (void)setCookieStorageMethod:(NSInteger)method {

  cookieStorageMethod_ = method;

  if (method == kGTMHTTPFetcherCookieStorageMethodSystemDefault) {
    // System default
    [request_ setHTTPShouldHandleCookies:YES];

    // No need for a cookie storage object
    self.cookieStorage = nil;

  } else {
    // Not system default
    [request_ setHTTPShouldHandleCookies:NO];

    if (method == kGTMHTTPFetcherCookieStorageMethodStatic) {
      // Store cookies in the static array
      NSAssert(gGTMFetcherStaticCookieStorage != nil,
               @"cookie storage requires GTMHTTPFetchHistory");

      self.cookieStorage = gGTMFetcherStaticCookieStorage;
    } else if (method == kGTMHTTPFetcherCookieStorageMethodFetchHistory) {
      // store cookies in the fetch history
      self.cookieStorage = [fetchHistory_ cookieStorage];
    } else {
      // kGTMHTTPFetcherCookieStorageMethodNone - ignore cookies
      self.cookieStorage = nil;
    }
  }
}

+ (id <GTMCookieStorageProtocol>)staticCookieStorage {
  return gGTMFetcherStaticCookieStorage;
}

+ (BOOL)doesSupportSentDataCallback {
#if GTM_IPHONE
  // NSURLConnection's didSendBodyData: delegate support appears to be
  // available starting in iPhone OS 3.0
  return (NSFoundationVersionNumber >= 678.47);
#else
  // Per WebKit's MaxFoundationVersionWithoutdidSendBodyDataDelegate
  //
  // Indicates if NSURLConnection will invoke the didSendBodyData: delegate
  // method
  return (NSFoundationVersionNumber > 677.21);
#endif
}

- (id <GTMHTTPFetchHistoryProtocol>)fetchHistory {
  return fetchHistory_;
}

- (void)setFetchHistory:(id <GTMHTTPFetchHistoryProtocol>)fetchHistory {
  [fetchHistory_ autorelease];
  fetchHistory_ = [fetchHistory retain];

  if (fetchHistory_ != nil) {
    // set the fetch history's cookie array to be the cookie store
    [self setCookieStorageMethod:kGTMHTTPFetcherCookieStorageMethodFetchHistory];

  } else {
    // The fetch history was removed
    if (cookieStorageMethod_ == kGTMHTTPFetcherCookieStorageMethodFetchHistory) {
      // Fall back to static storage
      [self setCookieStorageMethod:kGTMHTTPFetcherCookieStorageMethodStatic];
    }
  }
}

- (id)userData {
  @synchronized(self) {
    return userData_;
  }
}

- (void)setUserData:(id)theObj {
  @synchronized(self) {
    [userData_ autorelease];
    userData_ = [theObj retain];
  }
}

- (void)setProperties:(NSMutableDictionary *)dict {
  @synchronized(self) {
    [properties_ autorelease];

    // This copies rather than retains the parameter for compatiblity with
    // an earlier version that took an immutable parameter and copied it.
    properties_ = [dict mutableCopy];
  }
}

- (NSMutableDictionary *)properties {
  @synchronized(self) {
    return properties_;
  }
}

- (void)setProperty:(id)obj forKey:(NSString *)key {
  @synchronized(self) {
    if (properties_ == nil && obj != nil) {
      [self setProperties:[NSMutableDictionary dictionary]];
    }
    [properties_ setValue:obj forKey:key];
  }
}

- (id)propertyForKey:(NSString *)key {
  @synchronized(self) {
    return [properties_ objectForKey:key];
  }
}

- (void)addPropertiesFromDictionary:(NSDictionary *)dict {
  @synchronized(self) {
    if (properties_ == nil && dict != nil) {
      [self setProperties:[[dict mutableCopy] autorelease]];
    } else {
      [properties_ addEntriesFromDictionary:dict];
    }
  }
}

- (void)setCommentWithFormat:(id)format, ... {
#if !STRIP_GTM_FETCH_LOGGING
  NSString *result = format;
  if (format) {
    va_list argList;
    va_start(argList, format);

    result = [[[NSString alloc] initWithFormat:format
                                     arguments:argList] autorelease];
    va_end(argList);
  }
  [self setComment:result];
#endif
}

+ (Class)connectionClass {
  if (gGTMFetcherConnectionClass == nil) {
    gGTMFetcherConnectionClass = [NSURLConnection class];
  }
  return gGTMFetcherConnectionClass;
}

+ (void)setConnectionClass:(Class)theClass {
  gGTMFetcherConnectionClass = theClass;
}

#if STRIP_GTM_FETCH_LOGGING
+ (void)setLoggingEnabled:(BOOL)flag {
}
#endif // STRIP_GTM_FETCH_LOGGING

@end

void GTMAssertSelectorNilOrImplementedWithArgs(id obj, SEL sel, ...) {

  // Verify that the object's selector is implemented with the proper
  // number and type of arguments
#if DEBUG
  va_list argList;
  va_start(argList, sel);

  if (obj && sel) {
    // Check that the selector is implemented
    if (![obj respondsToSelector:sel]) {
      NSLog(@"\"%@\" selector \"%@\" is unimplemented or misnamed",
                             NSStringFromClass([obj class]),
                             NSStringFromSelector(sel));
      NSCAssert(0, @"callback selector unimplemented or misnamed");
    } else {
      const char *expectedArgType;
      unsigned int argCount = 2; // skip self and _cmd
      NSMethodSignature *sig = [obj methodSignatureForSelector:sel];

      // Check that each expected argument is present and of the correct type
      while ((expectedArgType = va_arg(argList, const char*)) != 0) {

        if ([sig numberOfArguments] > argCount) {
          const char *foundArgType = [sig getArgumentTypeAtIndex:argCount];

          if(0 != strncmp(foundArgType, expectedArgType, strlen(expectedArgType))) {
            NSLog(@"\"%@\" selector \"%@\" argument %d should be type %s",
                  NSStringFromClass([obj class]),
                  NSStringFromSelector(sel), (argCount - 2), expectedArgType);
            NSCAssert(0, @"callback selector argument type mistake");
          }
        }
        argCount++;
      }

      // Check that the proper number of arguments are present in the selector
      if (argCount != [sig numberOfArguments]) {
        NSLog( @"\"%@\" selector \"%@\" should have %d arguments",
                       NSStringFromClass([obj class]),
                       NSStringFromSelector(sel), (argCount - 2));
        NSCAssert(0, @"callback selector arguments incorrect");
      }
    }
  }

  va_end(argList);
#endif
}

NSString *GTMCleanedUserAgentString(NSString *str) {
  // Reference http://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html
  // and http://www-archive.mozilla.org/build/user-agent-strings.html

  if (str == nil) return nil;

  NSMutableString *result = [NSMutableString stringWithString:str];

  // Replace spaces with underscores
  [result replaceOccurrencesOfString:@" "
                          withString:@"_"
                             options:0
                               range:NSMakeRange(0, [result length])];

  // Delete http token separators and remaining whitespace
  static NSCharacterSet *charsToDelete = nil;
  if (charsToDelete == nil) {
    // Make a set of unwanted characters
    NSString *const kSeparators = @"()<>@,;:\\\"/[]?={}";

    NSMutableCharacterSet *mutableChars;
    mutableChars = [[[NSCharacterSet whitespaceAndNewlineCharacterSet] mutableCopy] autorelease];
    [mutableChars addCharactersInString:kSeparators];
    charsToDelete = [mutableChars copy]; // hang on to an immutable copy
  }

  while (1) {
    NSRange separatorRange = [result rangeOfCharacterFromSet:charsToDelete];
    if (separatorRange.location == NSNotFound) break;

    [result deleteCharactersInRange:separatorRange];
  };

  return result;
}

NSString *GTMSystemVersionString(void) {
  NSString *systemString = @"";

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
  // Mac build
  static NSString *savedSystemString = nil;
  if (savedSystemString == nil) {
    // With Gestalt inexplicably deprecated in 10.8, we're reduced to reading
    // the system plist file.
    NSString *const kPath = @"/System/Library/CoreServices/SystemVersion.plist";
    NSDictionary *plist = [NSDictionary dictionaryWithContentsOfFile:kPath];
    NSString *versString = [plist objectForKey:@"ProductVersion"];
    if ([versString length] == 0) {
      versString = @"10.?.?";
    }
    savedSystemString = [[NSString alloc] initWithFormat:@"MacOSX/%@", versString];
  }
  systemString = savedSystemString;
#elif TARGET_OS_IPHONE
  // Compiling against the iPhone SDK

  static NSString *savedSystemString = nil;
  if (savedSystemString == nil) {
    // Avoid the slowness of calling currentDevice repeatedly on the iPhone
    UIDevice* currentDevice = [UIDevice currentDevice];

    NSString *rawModel = [currentDevice model];
    NSString *model = GTMCleanedUserAgentString(rawModel);

    NSString *systemVersion = [currentDevice systemVersion];

    savedSystemString = [[NSString alloc] initWithFormat:@"%@/%@",
                         model, systemVersion]; // "iPod_Touch/2.2"
  }
  systemString = savedSystemString;

#elif (GTL_IPHONE || GDATA_IPHONE)
  // Compiling iOS libraries against the Mac SDK
  systemString = @"iPhone/x.x";

#elif defined(_SYS_UTSNAME_H)
  // Foundation-only build
  struct utsname unameRecord;
  uname(&unameRecord);

  systemString = [NSString stringWithFormat:@"%s/%s",
                  unameRecord.sysname, unameRecord.release]; // "Darwin/8.11.1"
#endif

  return systemString;
}

// Return a generic name and version for the current application; this avoids
// anonymous server transactions.
NSString *GTMApplicationIdentifier(NSBundle *bundle) {
  static NSString *sAppID = nil;
  if (sAppID != nil) return sAppID;

  // If there's a bundle ID, use that; otherwise, use the process name
  if (bundle == nil) {
    bundle = [NSBundle mainBundle];
  }

  NSString *identifier;
  NSString *bundleID = [bundle bundleIdentifier];
  if ([bundleID length] > 0) {
    identifier = bundleID;
  } else {
    // Fall back on the procname, prefixed by "proc" to flag that it's
    // autogenerated and perhaps unreliable
    NSString *procName = [[NSProcessInfo processInfo] processName];
    identifier = [NSString stringWithFormat:@"proc_%@", procName];
  }

  // Clean up whitespace and special characters
  identifier = GTMCleanedUserAgentString(identifier);

  // If there's a version number, append that
  NSString *version = [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  if ([version length] == 0) {
    version = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
  }

  // Clean up whitespace and special characters
  version = GTMCleanedUserAgentString(version);

  // Glue the two together (cleanup done above or else cleanup would strip the
  // slash)
  if ([version length] > 0) {
    identifier = [identifier stringByAppendingFormat:@"/%@", version];
  }

  sAppID = [identifier copy];
  return sAppID;
}

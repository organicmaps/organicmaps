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
//  GTMHTTPFetcher.h
//

// This is essentially a wrapper around NSURLConnection for POSTs and GETs.
// If setPostData: is called, then POST is assumed.
//
// When would you use this instead of NSURLConnection?
//
// - When you just want the result from a GET, POST, or PUT
// - When you want the "standard" behavior for connections (redirection handling
//   an so on)
// - When you want automatic retry on failures
// - When you want to avoid cookie collisions with Safari and other applications
// - When you are fetching resources with ETags and want to avoid the overhead
//   of repeated fetches of unchanged data
// - When you need to set a credential for the http operation
//
// This is assumed to be a one-shot fetch request; don't reuse the object
// for a second fetch.
//
// The fetcher may be created auto-released, in which case it will release
// itself after the fetch completion callback.  The fetcher is implicitly
// retained as long as a connection is pending.
//
// But if you may need to cancel the fetcher, retain it and have the delegate
// release the fetcher in the callbacks.
//
// Sample usage:
//
//  NSURLRequest *request = [NSURLRequest requestWithURL:myURL];
//  GTMHTTPFetcher* myFetcher = [GTMHTTPFetcher fetcherWithRequest:request];
//
//  // optional upload body data
//  [myFetcher setPostData:[postString dataUsingEncoding:NSUTF8StringEncoding]];
//
//  [myFetcher beginFetchWithDelegate:self
//                  didFinishSelector:@selector(myFetcher:finishedWithData:error:)];
//
//  Upon fetch completion, the callback selector is invoked; it should have
//  this signature (you can use any callback method name you want so long as
//  the signature matches this):
//
//  - (void)myFetcher:(GTMHTTPFetcher *)fetcher finishedWithData:(NSData *)retrievedData error:(NSError *)error;
//
//  The block callback version looks like:
//
//  [myFetcher beginFetchWithCompletionHandler:^(NSData *retrievedData, NSError *error) {
//    if (error != nil) {
//      // status code or network error
//    } else {
//      // succeeded
//    }
//  }];

//
// NOTE:  Fetches may retrieve data from the server even though the server
//        returned an error.  The failure selector is called when the server
//        status is >= 300, with an NSError having domain
//        kGTMHTTPFetcherStatusDomain and code set to the server status.
//
//        Status codes are at <http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html>
//
//
// Threading and queue support:
//
// Callbacks require either that the thread used to start the fetcher have a run
// loop spinning (typically the main thread), or that an NSOperationQueue be
// provided upon which the delegate callbacks will be called.  Starting with
// iOS 6 and Mac OS X 10.7, clients may simply create an operation queue for
// callbacks on a background thread:
//
//   fetcher.delegateQueue = [[[NSOperationQueue alloc] init] autorelease];
//
// or specify the main queue for callbacks on the main thread:
//
//   fetcher.delegateQueue = [NSOperationQueue mainQueue];
//
// The client may also re-dispatch from the callbacks and notifications to
// a known dispatch queue:
//
//  [myFetcher beginFetchWithCompletionHandler:^(NSData *retrievedData, NSError *error) {
//    if (error == nil) {
//      dispatch_async(myDispatchQueue, ^{
//        ...
//     });
//    }
//  }];
//
//
//
// Downloading to disk:
//
// To have downloaded data saved directly to disk, specify either a path for the
// downloadPath property, or a file handle for the downloadFileHandle property.
// When downloading to disk, callbacks will be passed a nil for the NSData*
// arguments.
//
//
// HTTP methods and headers:
//
// Alternative HTTP methods, like PUT, and custom headers can be specified by
// creating the fetcher with an appropriate NSMutableURLRequest
//
//
// Proxies:
//
// Proxy handling is invisible so long as the system has a valid credential in
// the keychain, which is normally true (else most NSURL-based apps would have
// difficulty.)  But when there is a proxy authetication error, the the fetcher
// will call the failedWithError: method with the NSURLChallenge in the error's
// userInfo. The error method can get the challenge info like this:
//
//  NSURLAuthenticationChallenge *challenge
//     = [[error userInfo] objectForKey:kGTMHTTPFetcherErrorChallengeKey];
//  BOOL isProxyChallenge = [[challenge protectionSpace] isProxy];
//
// If a proxy error occurs, you can ask the user for the proxy username/password
// and call fetcher's setProxyCredential: to provide those for the
// next attempt to fetch.
//
//
// Cookies:
//
// There are three supported mechanisms for remembering cookies between fetches.
//
// By default, GTMHTTPFetcher uses a mutable array held statically to track
// cookies for all instantiated fetchers. This avoids server cookies being set
// by servers for the application from interfering with Safari cookie settings,
// and vice versa.  The fetcher cookies are lost when the application quits.
//
// To rely instead on WebKit's global NSHTTPCookieStorage, call
// setCookieStorageMethod: with kGTMHTTPFetcherCookieStorageMethodSystemDefault.
//
// If the fetcher is created from a GTMHTTPFetcherService object
// then the cookie storage mechanism is set to use the cookie storage in the
// service object rather than the static storage.
//
//
// Fetching for periodic checks:
//
// The fetcher object tracks ETag headers from responses and
// provide an "If-None-Match" header. This allows the server to save
// bandwidth by providing a status message instead of repeated response
// data.
//
// To get this behavior, create the fetcher from an GTMHTTPFetcherService object
// and look for a fetch callback error with code 304
// (kGTMHTTPFetcherStatusNotModified) like this:
//
// - (void)myFetcher:(GTMHTTPFetcher *)fetcher finishedWithData:(NSData *)data error:(NSError *)error {
//    if ([error code] == kGTMHTTPFetcherStatusNotModified) {
//      // |data| is empty; use the data from the previous finishedWithData: for this URL
//    } else {
//      // handle other server status code
//    }
// }
//
//
// Monitoring received data
//
// The optional received data selector can be set with setReceivedDataSelector:
// and should have the signature
//
//  - (void)myFetcher:(GTMHTTPFetcher *)fetcher receivedData:(NSData *)dataReceivedSoFar;
//
// The number bytes received so far is available as [fetcher downloadedLength].
// This number may go down if a redirect causes the download to begin again from
// a new server.
//
// If supplied by the server, the anticipated total download size is available
// as [[myFetcher response] expectedContentLength] (and may be -1 for unknown
// download sizes.)
//
//
// Automatic retrying of fetches
//
// The fetcher can optionally create a timer and reattempt certain kinds of
// fetch failures (status codes 408, request timeout; 503, service unavailable;
// 504, gateway timeout; networking errors NSURLErrorTimedOut and
// NSURLErrorNetworkConnectionLost.)  The user may set a retry selector to
// customize the type of errors which will be retried.
//
// Retries are done in an exponential-backoff fashion (that is, after 1 second,
// 2, 4, 8, and so on.)
//
// Enabling automatic retries looks like this:
//  [myFetcher setRetryEnabled:YES];
//
// With retries enabled, the success or failure callbacks are called only
// when no more retries will be attempted. Calling the fetcher's stopFetching
// method will terminate the retry timer, without the finished or failure
// selectors being invoked.
//
// Optionally, the client may set the maximum retry interval:
//  [myFetcher setMaxRetryInterval:60.0]; // in seconds; default is 60 seconds
//                                        // for downloads, 600 for uploads
//
// Also optionally, the client may provide a callback selector to determine
// if a status code or other error should be retried.
//  [myFetcher setRetrySelector:@selector(myFetcher:willRetry:forError:)];
//
// If set, the retry selector should have the signature:
//   -(BOOL)fetcher:(GTMHTTPFetcher *)fetcher willRetry:(BOOL)suggestedWillRetry forError:(NSError *)error
// and return YES to set the retry timer or NO to fail without additional
// fetch attempts.
//
// The retry method may return the |suggestedWillRetry| argument to get the
// default retry behavior.  Server status codes are present in the
// error argument, and have the domain kGTMHTTPFetcherStatusDomain. The
// user's method may look something like this:
//
//  -(BOOL)myFetcher:(GTMHTTPFetcher *)fetcher willRetry:(BOOL)suggestedWillRetry forError:(NSError *)error {
//
//    // perhaps examine [error domain] and [error code], or [fetcher retryCount]
//    //
//    // return YES to start the retry timer, NO to proceed to the failure
//    // callback, or |suggestedWillRetry| to get default behavior for the
//    // current error domain and code values.
//    return suggestedWillRetry;
//  }



#pragma once

#import <Foundation/Foundation.h>

#if defined(GTL_TARGET_NAMESPACE)
  // we're using target namespace macros
  #import "GTLDefines.h"
#elif defined(GDATA_TARGET_NAMESPACE)
  #import "GDataDefines.h"
#else
  #if TARGET_OS_IPHONE
    #ifndef GTM_FOUNDATION_ONLY
      #define GTM_FOUNDATION_ONLY 1
    #endif
    #ifndef GTM_IPHONE
      #define GTM_IPHONE 1
    #endif
  #endif
#endif

#if TARGET_OS_IPHONE && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 40000)
  #define GTM_BACKGROUND_FETCHING 1
#endif

#undef _EXTERN
#undef _INITIALIZE_AS
#ifdef GTMHTTPFETCHER_DEFINE_GLOBALS
  #define _EXTERN
  #define _INITIALIZE_AS(x) =x
#else
  #if defined(__cplusplus)
    #define _EXTERN extern "C"
  #else
    #define _EXTERN extern
  #endif
  #define _INITIALIZE_AS(x)
#endif

// notifications
//
// fetch started and stopped, and fetch retry delay started and stopped
_EXTERN NSString* const kGTMHTTPFetcherStartedNotification           _INITIALIZE_AS(@"kGTMHTTPFetcherStartedNotification");
_EXTERN NSString* const kGTMHTTPFetcherStoppedNotification           _INITIALIZE_AS(@"kGTMHTTPFetcherStoppedNotification");
_EXTERN NSString* const kGTMHTTPFetcherRetryDelayStartedNotification _INITIALIZE_AS(@"kGTMHTTPFetcherRetryDelayStartedNotification");
_EXTERN NSString* const kGTMHTTPFetcherRetryDelayStoppedNotification _INITIALIZE_AS(@"kGTMHTTPFetcherRetryDelayStoppedNotification");

// callback constants
_EXTERN NSString* const kGTMHTTPFetcherErrorDomain       _INITIALIZE_AS(@"com.google.GTMHTTPFetcher");
_EXTERN NSString* const kGTMHTTPFetcherStatusDomain      _INITIALIZE_AS(@"com.google.HTTPStatus");
_EXTERN NSString* const kGTMHTTPFetcherErrorChallengeKey _INITIALIZE_AS(@"challenge");
_EXTERN NSString* const kGTMHTTPFetcherStatusDataKey     _INITIALIZE_AS(@"data");  // data returned with a kGTMHTTPFetcherStatusDomain error

enum {
  kGTMHTTPFetcherErrorDownloadFailed = -1,
  kGTMHTTPFetcherErrorAuthenticationChallengeFailed = -2,
  kGTMHTTPFetcherErrorChunkUploadFailed = -3,
  kGTMHTTPFetcherErrorFileHandleException = -4,
  kGTMHTTPFetcherErrorBackgroundExpiration = -6,

  // The code kGTMHTTPFetcherErrorAuthorizationFailed (-5) has been removed;
  // look for status 401 instead.

  kGTMHTTPFetcherStatusNotModified = 304,
  kGTMHTTPFetcherStatusBadRequest = 400,
  kGTMHTTPFetcherStatusUnauthorized = 401,
  kGTMHTTPFetcherStatusForbidden = 403,
  kGTMHTTPFetcherStatusPreconditionFailed = 412
};

// cookie storage methods
enum {
  kGTMHTTPFetcherCookieStorageMethodStatic = 0,
  kGTMHTTPFetcherCookieStorageMethodFetchHistory = 1,
  kGTMHTTPFetcherCookieStorageMethodSystemDefault = 2,
  kGTMHTTPFetcherCookieStorageMethodNone = 3
};

#ifdef __cplusplus
extern "C" {
#endif

void GTMAssertSelectorNilOrImplementedWithArgs(id obj, SEL sel, ...);

// Utility functions for applications self-identifying to servers via a
// user-agent header

// Make a proper app name without whitespace from the given string, removing
// whitespace and other characters that may be special parsed marks of
// the full user-agent string.
NSString *GTMCleanedUserAgentString(NSString *str);

// Make an identifier like "MacOSX/10.7.1" or "iPod_Touch/4.1"
NSString *GTMSystemVersionString(void);

// Make a generic name and version for the current application, like
// com.example.MyApp/1.2.3 relying on the bundle identifier and the
// CFBundleShortVersionString or CFBundleVersion.  If no bundle ID
// is available, the process name preceded by "proc_" is used.
NSString *GTMApplicationIdentifier(NSBundle *bundle);

#ifdef __cplusplus
}  // extern "C"
#endif

@class GTMHTTPFetcher;

@protocol GTMCookieStorageProtocol <NSObject>
// This protocol allows us to call into the service without requiring
// GTMCookieStorage sources in this project
//
// The public interface for cookie handling is the GTMCookieStorage class,
// accessible from a fetcher service object's fetchHistory or from the fetcher's
// +staticCookieStorage method.
- (NSArray *)cookiesForURL:(NSURL *)theURL;
- (void)setCookies:(NSArray *)newCookies;
@end

@protocol GTMHTTPFetchHistoryProtocol <NSObject>
// This protocol allows us to call the fetch history object without requiring
// GTMHTTPFetchHistory sources in this project
- (void)updateRequest:(NSMutableURLRequest *)request isHTTPGet:(BOOL)isHTTPGet;
- (BOOL)shouldCacheETaggedData;
- (NSData *)cachedDataForRequest:(NSURLRequest *)request;
- (id <GTMCookieStorageProtocol>)cookieStorage;
- (void)updateFetchHistoryWithRequest:(NSURLRequest *)request
                             response:(NSURLResponse *)response
                       downloadedData:(NSData *)downloadedData;
- (void)removeCachedDataForRequest:(NSURLRequest *)request;
@end

@protocol GTMHTTPFetcherServiceProtocol <NSObject>
// This protocol allows us to call into the service without requiring
// GTMHTTPFetcherService sources in this project

@property (retain) NSOperationQueue *delegateQueue;

- (BOOL)fetcherShouldBeginFetching:(GTMHTTPFetcher *)fetcher;
- (void)fetcherDidStop:(GTMHTTPFetcher *)fetcher;

- (GTMHTTPFetcher *)fetcherWithRequest:(NSURLRequest *)request;
- (BOOL)isDelayingFetcher:(GTMHTTPFetcher *)fetcher;
@end

@protocol GTMFetcherAuthorizationProtocol <NSObject>
@required
// This protocol allows us to call the authorizer without requiring its sources
// in this project
- (void)authorizeRequest:(NSMutableURLRequest *)request
                delegate:(id)delegate
       didFinishSelector:(SEL)sel;

- (void)stopAuthorization;

- (void)stopAuthorizationForRequest:(NSURLRequest *)request;

- (BOOL)isAuthorizingRequest:(NSURLRequest *)request;

- (BOOL)isAuthorizedRequest:(NSURLRequest *)request;

- (NSString *)userEmail;

@optional
@property (assign) id <GTMHTTPFetcherServiceProtocol> fetcherService; // WEAK

- (BOOL)primeForRefresh;
@end

// GTMHTTPFetcher objects are used for async retrieval of an http get or post
//
// See additional comments at the beginning of this file
@interface GTMHTTPFetcher : NSObject {
 @protected
  NSMutableURLRequest *request_;
  NSURLConnection *connection_;
  NSMutableData *downloadedData_;
  NSString *downloadPath_;
  NSString *temporaryDownloadPath_;
  NSFileHandle *downloadFileHandle_;
  unsigned long long downloadedLength_;
  NSURLCredential *credential_;     // username & password
  NSURLCredential *proxyCredential_; // credential supplied to proxy servers
  NSData *postData_;
  NSInputStream *postStream_;
  NSMutableData *loggedStreamData_;
  NSURLResponse *response_;         // set in connection:didReceiveResponse:
  id delegate_;
  SEL finishedSel_;                 // should by implemented by delegate
  SEL sentDataSel_;                 // optional, set with setSentDataSelector
  SEL receivedDataSel_;             // optional, set with setReceivedDataSelector
#if NS_BLOCKS_AVAILABLE
  void (^completionBlock_)(NSData *, NSError *);
  void (^receivedDataBlock_)(NSData *);
  void (^sentDataBlock_)(NSInteger, NSInteger, NSInteger);
  BOOL (^retryBlock_)(BOOL, NSError *);
#elif !__LP64__
  // placeholders: for 32-bit builds, keep the size of the object's ivar section
  // the same with and without blocks
  id completionPlaceholder_;
  id receivedDataPlaceholder_;
  id sentDataPlaceholder_;
  id retryPlaceholder_;
#endif
  BOOL hasConnectionEnded_;         // set if the connection need not be cancelled
  BOOL isCancellingChallenge_;      // set only when cancelling an auth challenge
  BOOL isStopNotificationNeeded_;   // set when start notification has been sent
  BOOL shouldFetchInBackground_;
#if GTM_BACKGROUND_FETCHING
  NSUInteger backgroundTaskIdentifer_; // UIBackgroundTaskIdentifier
#endif
  id userData_;                     // retained, if set by caller
  NSMutableDictionary *properties_; // more data retained for caller
  NSArray *runLoopModes_;           // optional
  NSOperationQueue *delegateQueue_; // optional; available iOS 6/10.7 and later
  id <GTMHTTPFetchHistoryProtocol> fetchHistory_; // if supplied by the caller, used for Last-Modified-Since checks and cookies
  NSInteger cookieStorageMethod_;   // constant from above
  id <GTMCookieStorageProtocol> cookieStorage_;

  id <GTMFetcherAuthorizationProtocol> authorizer_;

  // the service object that created and monitors this fetcher, if any
  id <GTMHTTPFetcherServiceProtocol> service_;
  NSString *serviceHost_;
  NSInteger servicePriority_;
  NSThread *thread_;

  BOOL isRetryEnabled_;             // user wants auto-retry
  SEL retrySel_;                    // optional; set with setRetrySelector
  NSTimer *retryTimer_;
  NSUInteger retryCount_;
  NSTimeInterval maxRetryInterval_; // default 600 seconds
  NSTimeInterval minRetryInterval_; // random between 1 and 2 seconds
  NSTimeInterval retryFactor_;      // default interval multiplier is 2
  NSTimeInterval lastRetryInterval_;
  BOOL hasAttemptedAuthRefresh_;

  NSString *comment_;               // comment for log
  NSString *log_;
#if !STRIP_GTM_FETCH_LOGGING
  NSString *logRequestBody_;
  NSString *logResponseBody_;
  BOOL shouldDeferResponseBodyLogging_;
#endif
}

// Create a fetcher
//
// fetcherWithRequest will return an autoreleased fetcher, but if
// the connection is successfully created, the connection should retain the
// fetcher for the life of the connection as well. So the caller doesn't have
// to retain the fetcher explicitly unless they want to be able to cancel it.
+ (GTMHTTPFetcher *)fetcherWithRequest:(NSURLRequest *)request;

// Convenience methods that make a request, like +fetcherWithRequest
+ (GTMHTTPFetcher *)fetcherWithURL:(NSURL *)requestURL;
+ (GTMHTTPFetcher *)fetcherWithURLString:(NSString *)requestURLString;

// Designated initializer
- (id)initWithRequest:(NSURLRequest *)request;

// Fetcher request
//
// The underlying request is mutable and may be modified by the caller
@property (retain) NSMutableURLRequest *mutableRequest;

// Setting the credential is optional; it is used if the connection receives
// an authentication challenge
@property (retain) NSURLCredential *credential;

// Setting the proxy credential is optional; it is used if the connection
// receives an authentication challenge from a proxy
@property (retain) NSURLCredential *proxyCredential;

// If post data or stream is not set, then a GET retrieval method is assumed
@property (retain) NSData *postData;
@property (retain) NSInputStream *postStream;

// The default cookie storage method is kGTMHTTPFetcherCookieStorageMethodStatic
// without a fetch history set, and kGTMHTTPFetcherCookieStorageMethodFetchHistory
// with a fetch history set
//
// Applications needing control of cookies across a sequence of fetches should
// create fetchers from a GTMHTTPFetcherService object (which encapsulates
// fetch history) for a well-defined cookie store
@property (assign) NSInteger cookieStorageMethod;

+ (id <GTMCookieStorageProtocol>)staticCookieStorage;

// Object to add authorization to the request, if needed
@property (retain) id <GTMFetcherAuthorizationProtocol> authorizer;

// The service object that created and monitors this fetcher, if any
@property (retain) id <GTMHTTPFetcherServiceProtocol> service;

// The host, if any, used to classify this fetcher in the fetcher service
@property (copy) NSString *serviceHost;

// The priority, if any, used for starting fetchers in the fetcher service
//
// Lower values are higher priority; the default is 0, and values may
// be negative or positive. This priority affects only the start order of
// fetchers that are being delayed by a fetcher service.
@property (assign) NSInteger servicePriority;

// The thread used to run this fetcher in the fetcher service when no operation
// queue is provided.
@property (retain) NSThread *thread;

// The delegate is retained during the connection
@property (retain) id delegate;

// On iOS 4 and later, the fetch may optionally continue while the app is in the
// background until finished or stopped by OS expiration
//
// The default value is NO
//
// For Mac OS X, background fetches are always supported, and this property
// is ignored
@property (assign) BOOL shouldFetchInBackground;

// The delegate's optional sentData selector may be used to monitor upload
// progress. It should have a signature like:
//  - (void)myFetcher:(GTMHTTPFetcher *)fetcher
//              didSendBytes:(NSInteger)bytesSent
//            totalBytesSent:(NSInteger)totalBytesSent
//  totalBytesExpectedToSend:(NSInteger)totalBytesExpectedToSend;
//
// +doesSupportSentDataCallback indicates if this delegate method is supported
+ (BOOL)doesSupportSentDataCallback;

@property (assign) SEL sentDataSelector;

// The delegate's optional receivedData selector may be used to monitor download
// progress. It should have a signature like:
//  - (void)myFetcher:(GTMHTTPFetcher *)fetcher
//       receivedData:(NSData *)dataReceivedSoFar;
//
// The dataReceived argument will be nil when downloading to a path or to a
// file handle.
//
// Applications should not use this method to accumulate the received data;
// the callback method or block supplied to the beginFetch call will have
// the complete NSData received.
@property (assign) SEL receivedDataSelector;

#if NS_BLOCKS_AVAILABLE
// The full interface to the block is provided rather than just a typedef for
// its parameter list in order to get more useful code completion in the Xcode
// editor
@property (copy) void (^sentDataBlock)(NSInteger bytesSent, NSInteger totalBytesSent, NSInteger bytesExpectedToSend);

// The dataReceived argument will be nil when downloading to a path or to
// a file handle
@property (copy) void (^receivedDataBlock)(NSData *dataReceivedSoFar);
#endif

// retrying; see comments at the top of the file.  Calling
// setRetryEnabled(YES) resets the min and max retry intervals.
@property (assign, getter=isRetryEnabled) BOOL retryEnabled;

// Retry selector or block is optional for retries.
//
// If present, it should have the signature:
//   -(BOOL)fetcher:(GTMHTTPFetcher *)fetcher willRetry:(BOOL)suggestedWillRetry forError:(NSError *)error
// and return YES to cause a retry.  See comments at the top of this file.
@property (assign) SEL retrySelector;

#if NS_BLOCKS_AVAILABLE
@property (copy) BOOL (^retryBlock)(BOOL suggestedWillRetry, NSError *error);
#endif

// Retry intervals must be strictly less than maxRetryInterval, else
// they will be limited to maxRetryInterval and no further retries will
// be attempted.  Setting maxRetryInterval to 0.0 will reset it to the
// default value, 600 seconds.

@property (assign) NSTimeInterval maxRetryInterval;

// Starting retry interval.  Setting minRetryInterval to 0.0 will reset it
// to a random value between 1.0 and 2.0 seconds.  Clients should normally not
// call this except for unit testing.
@property (assign) NSTimeInterval minRetryInterval;

// Multiplier used to increase the interval between retries, typically 2.0.
// Clients should not need to call this.
@property (assign) double retryFactor;

// Number of retries attempted
@property (readonly) NSUInteger retryCount;

// interval delay to precede next retry
@property (readonly) NSTimeInterval nextRetryInterval;

// Begin fetching the request
//
// The delegate can optionally implement the finished selectors or pass NULL
// for it.
//
// Returns YES if the fetch is initiated.  The delegate is retained between
// the beginFetch call until after the finish callback.
//
// An error is passed to the callback for server statuses 300 or
// higher, with the status stored as the error object's code.
//
// finishedSEL has a signature like:
//   - (void)fetcher:(GTMHTTPFetcher *)fetcher finishedWithData:(NSData *)data error:(NSError *)error;
//
// If the application has specified a downloadPath or downloadFileHandle
// for the fetcher, the data parameter passed to the callback will be nil.

- (BOOL)beginFetchWithDelegate:(id)delegate
             didFinishSelector:(SEL)finishedSEL;

#if NS_BLOCKS_AVAILABLE
- (BOOL)beginFetchWithCompletionHandler:(void (^)(NSData *data, NSError *error))handler;
#endif


// Returns YES if this is in the process of fetching a URL
- (BOOL)isFetching;

// Cancel the fetch of the request that's currently in progress
- (void)stopFetching;

// Return the status code from the server response
@property (readonly) NSInteger statusCode;

// Return the http headers from the response
@property (retain, readonly) NSDictionary *responseHeaders;

// The response, once it's been received
@property (retain) NSURLResponse *response;

// Bytes downloaded so far
@property (readonly) unsigned long long downloadedLength;

// Buffer of currently-downloaded data
@property (readonly, retain) NSData *downloadedData;

// Path in which to non-atomically create a file for storing the downloaded data
//
// The path must be set before fetching begins.  The download file handle
// will be created for the path, and can be used to monitor progress. If a file
// already exists at the path, it will be overwritten.
@property (copy) NSString *downloadPath;

// If downloadFileHandle is set, data received is immediately appended to
// the file handle rather than being accumulated in the downloadedData property
//
// The file handle supplied must allow writing and support seekToFileOffset:,
// and must be set before fetching begins.  Setting a download path will
// override the file handle property.
@property (retain) NSFileHandle *downloadFileHandle;

// The optional fetchHistory object is used for a sequence of fetchers to
// remember ETags, cache ETagged data, and store cookies.  Typically, this
// is set by a GTMFetcherService object when it creates a fetcher.
//
// Side effect: setting fetch history implicitly calls setCookieStorageMethod:
@property (retain) id <GTMHTTPFetchHistoryProtocol> fetchHistory;

// userData is retained for the convenience of the caller
@property (retain) id userData;

// Stored property values are retained for the convenience of the caller
@property (copy) NSMutableDictionary *properties;

- (void)setProperty:(id)obj forKey:(NSString *)key; // pass nil obj to remove property
- (id)propertyForKey:(NSString *)key;

- (void)addPropertiesFromDictionary:(NSDictionary *)dict;

// Comments are useful for logging
@property (copy) NSString *comment;

- (void)setCommentWithFormat:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);

// Log of request and response, if logging is enabled
@property (copy) NSString *log;

// Callbacks can be invoked on an operation queue rather than via the run loop,
// starting on 10.7 and iOS 6.  If a delegate queue is supplied. the run loop
// modes are ignored.
@property (retain) NSOperationQueue *delegateQueue;

// Using the fetcher while a modal dialog is displayed requires setting the
// run-loop modes to include NSModalPanelRunLoopMode
@property (retain) NSArray *runLoopModes;

// Users who wish to replace GTMHTTPFetcher's use of NSURLConnection
// can do so globally here.  The replacement should be a subclass of
// NSURLConnection.
+ (Class)connectionClass;
+ (void)setConnectionClass:(Class)theClass;

// Spin the run loop, discarding events, until the fetch has completed
//
// This is only for use in testing or in tools without a user interface.
//
// Synchronous fetches should never be done by shipping apps; they are
// sufficient reason for rejection from the app store.
- (void)waitForCompletionWithTimeout:(NSTimeInterval)timeoutInSeconds;

#if STRIP_GTM_FETCH_LOGGING
// if logging is stripped, provide a stub for the main method
// for controlling logging
+ (void)setLoggingEnabled:(BOOL)flag;
#endif // STRIP_GTM_FETCH_LOGGING

@end

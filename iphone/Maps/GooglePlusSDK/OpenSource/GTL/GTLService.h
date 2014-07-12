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
//  GTLService.h
//

// Service object documentation:
// https://code.google.com/p/google-api-objectivec-client/wiki/Introduction#Services_and_Tickets

#import <Foundation/Foundation.h>

#import "GTLDefines.h"
#import "GTMHTTPFetcherService.h"
#import "GTLBatchQuery.h"
#import "GTLBatchResult.h"
#import "GTLDateTime.h"
#import "GTLErrorObject.h"
#import "GTLFramework.h"
#import "GTLJSONParser.h"
#import "GTLObject.h"
#import "GTLQuery.h"
#import "GTLUtilities.h"

#undef _EXTERN
#undef _INITIALIZE_AS
#ifdef GTLSERVICE_DEFINE_GLOBALS
#define _EXTERN
#define _INITIALIZE_AS(x) =x
#else
#define _EXTERN extern
#define _INITIALIZE_AS(x)
#endif

// Error domains
_EXTERN NSString* const kGTLServiceErrorDomain _INITIALIZE_AS(@"com.google.GTLServiceDomain");
enum {
  kGTLErrorQueryResultMissing = -3000,
  kGTLErrorWaitTimedOut       = -3001
};

_EXTERN NSString* const kGTLJSONRPCErrorDomain _INITIALIZE_AS(@"com.google.GTLJSONRPCErrorDomain");

// We'll consistently store the server error string in the userInfo under
// this key
_EXTERN NSString* const kGTLServerErrorStringKey _INITIALIZE_AS(@"error");

_EXTERN Class const kGTLUseRegisteredClass _INITIALIZE_AS(nil);

_EXTERN NSUInteger const kGTLStandardUploadChunkSize _INITIALIZE_AS(NSUIntegerMax);

// When servers return us structured JSON errors, the NSError will
// contain a GTLErrorObject in the userInfo dictionary under the key
// kGTLStructuredErrorsKey
_EXTERN NSString* const kGTLStructuredErrorKey _INITIALIZE_AS(@"GTLStructuredError");

// When specifying an ETag for updating or deleting a single entry, use
// kGTLETagWildcard to tell the server to replace the current value
// unconditionally.  Do not use this in entries in a batch feed.
_EXTERN NSString* const kGTLETagWildcard _INITIALIZE_AS(@"*");

// Notifications when parsing of a fetcher feed or entry begins or ends
_EXTERN NSString* const kGTLServiceTicketParsingStartedNotification _INITIALIZE_AS(@"kGTLServiceTicketParsingStartedNotification");
_EXTERN NSString* const kGTLServiceTicketParsingStoppedNotification _INITIALIZE_AS(@"kGTLServiceTicketParsingStoppedNotification");

@class GTLServiceTicket;

// Block types used for fetch callbacks
//
// These typedefs are not used in the header file method declarations
// since it's more useful when code sense expansions show the argument
// types rather than the typedefs

#if NS_BLOCKS_AVAILABLE
typedef void (^GTLServiceCompletionHandler)(GTLServiceTicket *ticket, id object, NSError *error);

typedef void (^GTLServiceUploadProgressBlock)(GTLServiceTicket *ticket, unsigned long long numberOfBytesRead, unsigned long long dataLength);
#else
typedef void *GTLServiceCompletionHandler;

typedef void *GTLServiceUploadProgressBlock;
#endif // NS_BLOCKS_AVAILABLE

#pragma mark -

//
// Service base class
//

@interface GTLService : NSObject {
 @private
  NSOperationQueue *parseQueue_;
  NSString *userAgent_;
  GTMHTTPFetcherService *fetcherService_;
  NSString *userAgentAddition_;

  NSMutableDictionary *serviceProperties_; // initial values for properties in future tickets

  NSDictionary *surrogates_; // initial value for surrogates in future tickets

  SEL uploadProgressSelector_; // optional

#if NS_BLOCKS_AVAILABLE
  BOOL (^retryBlock_)(GTLServiceTicket *, BOOL, NSError *);
  void (^uploadProgressBlock_)(GTLServiceTicket *ticket,
                               unsigned long long numberOfBytesRead,
                               unsigned long long dataLength);
#elif !__LP64__
  // Placeholders: for 32-bit builds, keep the size of the object's ivar section
  // the same with and without blocks
  id retryPlaceholder_;
  id uploadProgressPlaceholder_;
#endif

  NSUInteger uploadChunkSize_;      // zero when uploading via multi-part MIME http body

  BOOL isRetryEnabled_;             // user allows auto-retries
  SEL retrySelector_;               // optional; set with setServiceRetrySelector
  NSTimeInterval maxRetryInterval_; // default to 600. seconds

  BOOL shouldFetchNextPages_;

  NSString *apiKey_;
  BOOL isRESTDataWrapperRequired_;
  NSString *apiVersion_;
  NSURL *rpcURL_;
  NSURL *rpcUploadURL_;
  NSDictionary *urlQueryParameters_;
  NSDictionary *additionalHTTPHeaders_;
}

#pragma mark Query Execution

// The finishedSelector has a signature matching:
//
//   - (void)serviceTicket:(GTLServiceTicket *)ticket
//      finishedWithObject:(GTLObject *)object
//                   error:(NSError *)error
//
// If an error occurs, the error parameter will be non-nil.  Otherwise,
// the object parameter will point to a GTLObject, if any was returned by
// the fetch.  (Delete fetches return no object, so the second parameter will
// be nil.)
//
// If the query object is a GTLBatchQuery, the object passed to the callback
// will be a GTLBatchResult; see the batch query documentation:
// https://code.google.com/p/google-api-objectivec-client/wiki/Introduction#Batch_Operations

- (GTLServiceTicket *)executeQuery:(id<GTLQueryProtocol>)query
                          delegate:(id)delegate
                 didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)executeQuery:(id<GTLQueryProtocol>)query
                 completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));
#endif

// Automatic page fetches
//
// Tickets can optionally do a sequence of fetches for queries where
// repeated requests with nextPageToken or nextStartIndex values is required to
// retrieve items of all pages of the response collection.  The client's
// callback is invoked only when all items have been retrieved, or an error has
// occurred.  During the fetch, the items accumulated so far are available from
// the ticket.
//
// Note that the final object may be a combination of multiple page responses
// so it may not be the same as if all results had been returned in a single
// page. Some fields of the response such as total item counts may reflect only
// the final page's values.
//
// Automatic page fetches will return an error if more than 25 page fetches are
// required.  For debug builds, this will log a warning to the console when more
// than 2 page fetches occur, as a reminder that the query's maxResults
// parameter should probably be increased to specify more items returned per
// page.
//
// Default value is NO.
@property (nonatomic, assign) BOOL shouldFetchNextPages;

// Retrying; see comments on retry support at the top of GTMHTTPFetcher.
//
// Default value is NO.
@property (nonatomic, assign, getter=isRetryEnabled) BOOL retryEnabled;

// Some services require a developer key for quotas and limits.  Setting this
// will include it on all request sent to this service via a GTLQuery class.
@property (nonatomic, copy) NSString *APIKey;

// An authorizer adds user authentication headers to the request as needed.
@property (nonatomic, retain) id <GTMFetcherAuthorizationProtocol> authorizer;

// Retry selector is optional for retries.
//
// If present, it should have the signature:
//   -(BOOL)ticket:(GTLServiceTicket *)ticket willRetry:(BOOL)suggestedWillRetry forError:(NSError *)error
// and return YES to cause a retry.  Note that unlike the GTMHTTPFetcher retry
// selector, this selector's first argument is a ticket, not a fetcher.

@property (nonatomic, assign) SEL retrySelector;
#if NS_BLOCKS_AVAILABLE
@property (copy) BOOL (^retryBlock)(GTLServiceTicket *ticket, BOOL suggestedWillRetry, NSError *error);
#endif

@property (nonatomic, assign) NSTimeInterval maxRetryInterval;

//
// Fetches may be done using RPC or REST APIs, without creating
// a GTLQuery object
//

#pragma mark RPC Fetch Methods

//
// These methods may be used for RPC fetches without creating a GTLQuery object
//

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));
#endif

#pragma mark REST Fetch Methods

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)objectURL
                                delegate:(id)delegate
                       didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)objectURL
                             objectClass:(Class)objectClass
                                delegate:(id)delegate
                       didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

- (GTLServiceTicket *)fetchPublicObjectWithURL:(NSURL *)objectURL
                                   objectClass:(Class)objectClass
                                      delegate:(id)delegate
                             didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectByInsertingObject:(GTLObject *)bodyToPut
                                            forURL:(NSURL *)destinationURL
                                          delegate:(id)delegate
                                 didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1,2));

- (GTLServiceTicket *)fetchObjectByUpdatingObject:(GTLObject *)bodyToPut
                                           forURL:(NSURL *)destinationURL
                                         delegate:(id)delegate
                                didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1,2));

- (GTLServiceTicket *)deleteResourceURL:(NSURL *)destinationURL
                                   ETag:(NSString *)etagOrNil
                               delegate:(id)delegate
                      didFinishSelector:(SEL)finishedSelector GTL_NONNULL((1));

#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)objectURL
                       completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectByInsertingObject:(GTLObject *)bodyToPut
                                            forURL:(NSURL *)destinationURL
                                 completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));

- (GTLServiceTicket *)fetchObjectByUpdatingObject:(GTLObject *)bodyToPut
                                           forURL:(NSURL *)destinationURL
                                completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));

- (GTLServiceTicket *)deleteResourceURL:(NSURL *)destinationURL
                                   ETag:(NSString *)etagOrNil
                      completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler GTL_NONNULL((1));
#endif

#pragma mark User Properties

// Properties and userData are supported for client convenience.
//
// Property keys beginning with _ are reserved by the library.
//
// The service properties dictionary is copied to become the initial property
// dictionary for each ticket.
- (void)setServiceProperty:(id)obj forKey:(NSString *)key GTL_NONNULL((2)); // pass nil obj to remove property
- (id)servicePropertyForKey:(NSString *)key GTL_NONNULL((1));

@property (nonatomic, copy) NSDictionary *serviceProperties;

// The service userData becomes the initial value for each future ticket's
// userData.
@property (nonatomic, retain) id serviceUserData;

#pragma mark Request Settings

// Set the surrogates to be used for future tickets.  Surrogates are subclasses
// to be used instead of standard classes when creating objects from the JSON.
// For example, this code will make the framework generate objects
// using MyCalendarItemSubclass instead of GTLItemCalendar and
// MyCalendarEventSubclass instead of GTLItemCalendarEvent.
//
//  NSDictionary *surrogates = [NSDictionary dictionaryWithObjectsAndKeys:
//    [MyCalendarEntrySubclass class], [GTLItemCalendar class],
//    [MyCalendarEventSubclass class], [GTLItemCalendarEvent class],
//    nil];
//  [calendarService setServiceSurrogates:surrogates];
//
@property (nonatomic, retain) NSDictionary *surrogates;

// On iOS 4 and later, the fetch may optionally continue in the background
// until finished or stopped by OS expiration.
//
// The default value is NO.
//
// For Mac OS X, background fetches are always supported, and this property
// is ignored.
@property (nonatomic, assign) BOOL shouldFetchInBackground;

// Callbacks can be invoked on an operation queue rather than via the run loop
// starting on 10.7 and iOS 6.  Do not specify both run loop modes and an
// operation queue. Specifying a delegate queue typically looks like this:
//
//   service.delegateQueue = [[[NSOperationQueue alloc] init] autorelease];
//
// Since the callbacks will be on a thread of the operation queue, the client
// may re-dispatch from the callbacks to a known dispatch queue or to the
// main queue.
@property (nonatomic, retain) NSOperationQueue *delegateQueue;

// Run loop modes are used for scheduling NSURLConnections.
//
// The default value, nil, schedules connections using the current run
// loop mode.  To use the service during a modal dialog, be sure to specify
// NSModalPanelRunLoopMode as one of the modes.
@property (nonatomic, retain) NSArray *runLoopModes;

// Applications needing an additional identifier in the server logs may specify
// one.
@property (nonatomic, copy) NSString *userAgentAddition;

// Applications have a default user-agent based on the application signature
// in the Info.plist settings.  Most applications should not explicitly set
// this property.
@property (nonatomic, copy) NSString *userAgent;

// The request user agent includes the library and OS version appended to the
// base userAgent, along with the optional addition string.
@property (nonatomic, readonly) NSString *requestUserAgent;

// Applications may call requestForURL:httpMethod to get a request with the
// proper user-agent and ETag headers
//
// For http method, pass nil (for default GET method), POST, PUT, or DELETE
- (NSMutableURLRequest *)requestForURL:(NSURL *)url
                                  ETag:(NSString *)etagOrNil
                            httpMethod:(NSString *)httpMethodOrNil GTL_NONNULL((1));

// objectRequestForURL returns an NSMutableURLRequest for a JSON GTL object
//
// The object is the object being sent to the server, or nil;
// the http method may be nil for GET, or POST, PUT, DELETE
- (NSMutableURLRequest *)objectRequestForURL:(NSURL *)url
                                      object:(GTLObject *)object
                                        ETag:(NSString *)etag
                                  httpMethod:(NSString *)httpMethod
                                      isREST:(BOOL)isREST
                           additionalHeaders:(NSDictionary *)additionalHeaders
                                      ticket:(GTLServiceTicket *)ticket GTL_NONNULL((1));

// The queue used for parsing JSON responses (previously this property
// was called operationQueue)
@property (nonatomic, retain) NSOperationQueue *parseQueue;

// The fetcher service object issues the GTMHTTPFetcher instances
// for this API service
@property (nonatomic, retain) GTMHTTPFetcherService *fetcherService;

// Default storage for cookies is in the service object's fetchHistory.
//
// Apps that want to share cookies between all standalone fetchers and the
// service object may specify static application-wide cookie storage,
// kGTMHTTPFetcherCookieStorageMethodStatic.
@property (nonatomic, assign) NSInteger cookieStorageMethod;

// When sending REST style queries, should the payload be wrapped in a "data"
// element, and will the reply be wrapped in an "data" element.
@property (nonatomic, assign) BOOL isRESTDataWrapperRequired;

// Any url query parameters to add to urls (useful for debugging with some
// services).
@property (copy) NSDictionary *urlQueryParameters;

// Any extra http headers to set on requests for GTLObjects.
@property (copy) NSDictionary *additionalHTTPHeaders;

// The service API version.
@property (nonatomic, copy) NSString *apiVersion;

// The URL for sending RPC requests for this service.
@property (nonatomic, retain) NSURL *rpcURL;

// The URL for sending RPC requests which initiate file upload.
@property (nonatomic, retain) NSURL *rpcUploadURL;

// Set a non-zero value to enable uploading via chunked fetches
// (resumable uploads); typically this defaults to kGTLStandardUploadChunkSize
// for service subclasses that support chunked uploads
@property (nonatomic, assign) NSUInteger serviceUploadChunkSize;

// Service subclasses may specify their own default chunk size
+ (NSUInteger)defaultServiceUploadChunkSize;

// The service uploadProgressSelector becomes the initial value for each future
// ticket's uploadProgressSelector.
//
// The optional uploadProgressSelector will be called in the delegate as bytes
// are uploaded to the server.  It should have a signature matching
//
// - (void)ticket:(GTLServiceTicket *)ticket
//   hasDeliveredByteCount:(unsigned long long)numberOfBytesRead
//        ofTotalByteCount:(unsigned long long)dataLength;
@property (nonatomic, assign) SEL uploadProgressSelector;

#if NS_BLOCKS_AVAILABLE
@property (copy) void (^uploadProgressBlock)(GTLServiceTicket *ticket, unsigned long long numberOfBytesRead, unsigned long long dataLength);
#endif

// Wait synchronously for fetch to complete (strongly discouraged)
//
// This just runs the current event loop until the fetch completes
// or the timout limit is reached.  This may discard unexpected events
// that occur while spinning, so it's really not appropriate for use
// in serious applications.
//
// Returns true if an object was successfully fetched.  If the wait
// timed out, returns false and the returned error is nil.
//
// The returned object or error, if any, will be already autoreleased
//
// This routine will likely be removed in some future releases of the library.
- (BOOL)waitForTicket:(GTLServiceTicket *)ticket
              timeout:(NSTimeInterval)timeoutInSeconds
        fetchedObject:(GTLObject **)outObjectOrNil
                error:(NSError **)outErrorOrNil GTL_NONNULL((1));
@end

#pragma mark -

//
// Ticket base class
//
@interface GTLServiceTicket : NSObject {
  GTLService *service_;

  NSMutableDictionary *ticketProperties_;
  NSDictionary *surrogates_;

  GTMHTTPFetcher *objectFetcher_;
  SEL uploadProgressSelector_;
  BOOL shouldFetchNextPages_;
  BOOL isRetryEnabled_;
  SEL retrySelector_;
  NSTimeInterval maxRetryInterval_;

#if NS_BLOCKS_AVAILABLE
  BOOL (^retryBlock_)(GTLServiceTicket *, BOOL, NSError *);
  void (^uploadProgressBlock_)(GTLServiceTicket *ticket,
                               unsigned long long numberOfBytesRead,
                               unsigned long long dataLength);
#elif !__LP64__
  // Placeholders: for 32-bit builds, keep the size of the object's ivar section
  // the same with and without blocks
  id retryPlaceholder_;
  id uploadProgressPlaceholder_;
#endif

  GTLObject *postedObject_;
  GTLObject *fetchedObject_;
  id<GTLQueryProtocol> executingQuery_;
  id<GTLQueryProtocol> originalQuery_;
  NSError *fetchError_;
  BOOL hasCalledCallback_;
  NSUInteger pagesFetchedCounter_;

  NSString *apiKey_;
  BOOL isREST_;

  NSOperation *parseOperation_;
}

+ (id)ticketForService:(GTLService *)service;

- (id)initWithService:(GTLService *)service;

- (id)service;

#pragma mark Execution Control
// if cancelTicket is called, the fetch is stopped if it is in progress,
// the callbacks will not be called, and the ticket will no longer be useful
// (though the client must still release the ticket if it retained the ticket)
- (void)cancelTicket;

// chunked upload tickets may be paused
- (void)pauseUpload;
- (void)resumeUpload;
- (BOOL)isUploadPaused;

@property (nonatomic, retain) GTMHTTPFetcher *objectFetcher;
@property (nonatomic, assign) SEL uploadProgressSelector;

// Services which do not require an user authorization may require a developer
// API key for quota management
@property (nonatomic, copy) NSString *APIKey;

#pragma mark User Properties

// Properties and userData are supported for client convenience.
//
// Property keys beginning with _ are reserved by the library.
- (void)setProperty:(id)obj forKey:(NSString *)key GTL_NONNULL((1)); // pass nil obj to remove property
- (id)propertyForKey:(NSString *)key;

@property (nonatomic, copy) NSDictionary *properties;
@property (nonatomic, retain) id userData;

#pragma mark Payload

@property (nonatomic, retain) GTLObject *postedObject;
@property (nonatomic, retain) GTLObject *fetchedObject;
@property (nonatomic, retain) id<GTLQueryProtocol> executingQuery; // Query currently being fetched by this ticket
@property (nonatomic, retain) id<GTLQueryProtocol> originalQuery;  // Query used to create this ticket
- (GTLQuery *)queryForRequestID:(NSString *)requestID GTL_NONNULL((1)); // Returns the query from within the batch with the given id.

@property (nonatomic, retain) NSDictionary *surrogates;

#pragma mark Retry

@property (nonatomic, assign, getter=isRetryEnabled) BOOL retryEnabled;
@property (nonatomic, assign) SEL retrySelector;
#if NS_BLOCKS_AVAILABLE
@property (copy) BOOL (^retryBlock)(GTLServiceTicket *ticket, BOOL suggestedWillRetry, NSError *error);
#endif
@property (nonatomic, assign) NSTimeInterval maxRetryInterval;

#pragma mark Status

@property (nonatomic, readonly) NSInteger statusCode; // server status from object fetch
@property (nonatomic, retain) NSError *fetchError;
@property (nonatomic, assign) BOOL hasCalledCallback;

#pragma mark Pagination

@property (nonatomic, assign) BOOL shouldFetchNextPages;
@property (nonatomic, assign) NSUInteger pagesFetchedCounter;

#pragma mark Upload

#if NS_BLOCKS_AVAILABLE
@property (copy) void (^uploadProgressBlock)(GTLServiceTicket *ticket, unsigned long long numberOfBytesRead, unsigned long long dataLength);
#endif

@end


// Category to provide opaque access to tickets stored in fetcher properties
@interface GTMHTTPFetcher (GTLServiceTicketAdditions)
- (id)ticket;
@end


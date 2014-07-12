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
//  GTLService.m
//

#import <TargetConditionals.h>
#if TARGET_OS_MAC
#include <sys/utsname.h>
#endif

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#define GTLSERVICE_DEFINE_GLOBALS 1
#import "GTLService.h"

static NSString *const kUserDataPropertyKey = @"_userData";

static NSString* const kFetcherDelegateKey             = @"_delegate";
static NSString* const kFetcherObjectClassKey          = @"_objectClass";
static NSString* const kFetcherFinishedSelectorKey     = @"_finishedSelector";
static NSString* const kFetcherCompletionHandlerKey    = @"_completionHandler";
static NSString* const kFetcherTicketKey               = @"_ticket";
static NSString* const kFetcherFetchErrorKey           = @"_fetchError";
static NSString* const kFetcherParsingNotificationKey  = @"_parseNotification";
static NSString* const kFetcherParsedObjectKey         = @"_parsedObject";
static NSString* const kFetcherBatchClassMapKey        = @"_batchClassMap";
static NSString* const kFetcherCallbackThreadKey       = @"_callbackThread";
static NSString* const kFetcherCallbackRunLoopModesKey = @"_runLoopModes";

static const NSUInteger kMaxNumberOfNextPagesFetched = 25;

// we'll enforce 50K chunks minimum just to avoid the server getting hit
// with too many small upload chunks
static const NSUInteger kMinimumUploadChunkSize = 50000;
static const NSUInteger kStandardUploadChunkSize = NSUIntegerMax;

// Helper to get the ETag if it is defined on an object.
static NSString *ETagIfPresent(GTLObject *obj) {
  NSString *result = [obj.JSON objectForKey:@"etag"];
  return result;
}

@interface GTLServiceTicket ()
@property (retain) NSOperation *parseOperation;
@property (assign) BOOL isREST;
@end

// category to provide opaque access to tickets stored in fetcher properties
@implementation GTMHTTPFetcher (GTLServiceTicketAdditions)
- (id)ticket {
  return [self propertyForKey:kFetcherTicketKey];
}
@end

// If GTMHTTPUploadFetcher is available, it can be used for chunked uploads
//
// We locally declare some methods of GTMHTTPUploadFetcher so we
// do not need to import the header, as some projects may not have it available
@interface GTMHTTPUploadFetcher : GTMHTTPFetcher
+ (GTMHTTPUploadFetcher *)uploadFetcherWithRequest:(NSURLRequest *)request
                                        uploadData:(NSData *)data
                                    uploadMIMEType:(NSString *)uploadMIMEType
                                         chunkSize:(NSUInteger)chunkSize
                                    fetcherService:(GTMHTTPFetcherService *)fetcherService;
+ (GTMHTTPUploadFetcher *)uploadFetcherWithRequest:(NSURLRequest *)request
                                  uploadFileHandle:(NSFileHandle *)uploadFileHandle
                                    uploadMIMEType:(NSString *)uploadMIMEType
                                         chunkSize:(NSUInteger)chunkSize
                                    fetcherService:(GTMHTTPFetcherService *)fetcherService;
+ (GTMHTTPUploadFetcher *)uploadFetcherWithLocation:(NSURL *)location
                                   uploadFileHandle:(NSFileHandle *)fileHandle
                                     uploadMIMEType:(NSString *)uploadMIMEType
                                          chunkSize:(NSUInteger)chunkSize
                                     fetcherService:(GTMHTTPFetcherService *)fetcherService;
- (void)pauseFetching;
- (void)resumeFetching;
- (BOOL)isPaused;
@end


@interface GTLService ()
- (void)prepareToParseObjectForFetcher:(GTMHTTPFetcher *)fetcher;
- (void)handleParsedObjectForFetcher:(GTMHTTPFetcher *)fetcher;
- (BOOL)fetchNextPageWithQuery:(GTLQuery *)query
                      delegate:(id)delegate
           didFinishedSelector:(SEL)finishedSelector
             completionHandler:(GTLServiceCompletionHandler)completionHandler
                        ticket:(GTLServiceTicket *)ticket;
- (id <GTLQueryProtocol>)nextPageQueryForQuery:(GTLQuery *)query
                                        result:(GTLObject *)object
                                        ticket:(GTLServiceTicket *)ticket;
- (GTLObject *)mergedNewResultObject:(GTLObject *)newResult
                     oldResultObject:(GTLObject *)oldResult
                            forQuery:(GTLQuery *)query;
- (GTMHTTPUploadFetcher *)uploadFetcherWithRequest:(NSURLRequest *)request
                                    fetcherService:(GTMHTTPFetcherService *)fetcherService
                                            params:(GTLUploadParameters *)uploadParams;
+ (void)invokeCallback:(SEL)callbackSel
                target:(id)target
                ticket:(id)ticket
                object:(id)object
                 error:(id)error;
- (BOOL)invokeRetrySelector:(SEL)retrySelector
                   delegate:(id)delegate
                     ticket:(GTLServiceTicket *)ticket
                  willRetry:(BOOL)willRetry
                      error:(NSError *)error;
- (BOOL)objectFetcher:(GTMHTTPFetcher *)fetcher
            willRetry:(BOOL)willRetry
             forError:(NSError *)error;
- (void)objectFetcher:(GTMHTTPFetcher *)fetcher
     finishedWithData:(NSData *)data
                error:(NSError *)error;
- (void)parseObjectFromDataOfFetcher:(GTMHTTPFetcher *)fetcher;
@end

@interface GTLObject (StandardProperties)
@property (retain) NSString *ETag;
@property (retain) NSString *nextPageToken;
@property (retain) NSNumber *nextStartIndex;
@end

@implementation GTLService

@synthesize userAgentAddition = userAgentAddition_,
            fetcherService = fetcherService_,
            parseQueue = parseQueue_,
            shouldFetchNextPages = shouldFetchNextPages_,
            surrogates = surrogates_,
            uploadProgressSelector = uploadProgressSelector_,
            retryEnabled = isRetryEnabled_,
            retrySelector = retrySelector_,
            maxRetryInterval = maxRetryInterval_,
            APIKey = apiKey_,
            isRESTDataWrapperRequired = isRESTDataWrapperRequired_,
            urlQueryParameters = urlQueryParameters_,
            additionalHTTPHeaders = additionalHTTPHeaders_,
            apiVersion = apiVersion_,
            rpcURL = rpcURL_,
            rpcUploadURL = rpcUploadURL_;

#if NS_BLOCKS_AVAILABLE
@synthesize retryBlock = retryBlock_,
            uploadProgressBlock = uploadProgressBlock_;
#endif

+ (Class)ticketClass {
  return [GTLServiceTicket class];
}

- (id)init {
  self = [super init];
  if (self) {

#if GTL_IPHONE || (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5)
    // For 10.6 and up, always use an operation queue
    parseQueue_ = [[NSOperationQueue alloc] init];
#elif !GTL_SKIP_PARSE_THREADING
    // Avoid NSOperationQueue prior to 10.5.6, per
    // http://www.mikeash.com/?page=pyblog/use-nsoperationqueue.html
    SInt32 bcdSystemVersion = 0;
    (void) Gestalt(gestaltSystemVersion, &bcdSystemVersion);

    if (bcdSystemVersion >= 0x1057) {
      parseQueue_ = [[NSOperationQueue alloc] init];
    }
#else
    // parseQueue_ defaults to nil, so parsing will be done immediately
    // on the current thread
#endif

    fetcherService_ = [[GTMHTTPFetcherService alloc] init];

    NSUInteger chunkSize = [[self class] defaultServiceUploadChunkSize];
    self.serviceUploadChunkSize = chunkSize;
  }
  return self;
}

- (void)dealloc {
  [parseQueue_ release];
  [userAgent_ release];
  [fetcherService_ release];
  [userAgentAddition_ release];
  [serviceProperties_ release];
  [surrogates_ release];
#if NS_BLOCKS_AVAILABLE
  [uploadProgressBlock_ release];
  [retryBlock_ release];
#endif
  [apiKey_ release];
  [apiVersion_ release];
  [rpcURL_ release];
  [rpcUploadURL_ release];
  [urlQueryParameters_ release];
  [additionalHTTPHeaders_ release];

  [super dealloc];
}

- (NSString *)requestUserAgent {
  NSString *userAgent = self.userAgent;
  if ([userAgent length] == 0) {
    // the service instance is missing an explicit user-agent; use the bundle ID
    // or process name
    NSBundle *owningBundle = [NSBundle bundleForClass:[self class]];
    if (owningBundle == nil
        || [[owningBundle bundleIdentifier] isEqual:@"com.google.GTLFramework"]) {

      owningBundle = [NSBundle mainBundle];
    }

    userAgent = GTMApplicationIdentifier(owningBundle);
  }

  NSString *requestUserAgent = userAgent;

  // if the user agent already specifies the library version, we'll
  // use it verbatim in the request
  NSString *libraryString = @"google-api-objc-client";
  NSRange libRange = [userAgent rangeOfString:libraryString
                                      options:NSCaseInsensitiveSearch];
  if (libRange.location == NSNotFound) {
    // the user agent doesn't specify the client library, so append that
    // information, and the system version
    NSString *libVersionString = GTLFrameworkVersionString();

    NSString *systemString = GTMSystemVersionString();

    // We don't clean this with GTMCleanedUserAgentString so spaces are
    // preserved
    NSString *userAgentAddition = self.userAgentAddition;
    NSString *customString = userAgentAddition ?
      [@" " stringByAppendingString:userAgentAddition] : @"";

    // Google servers look for gzip in the user agent before sending gzip-
    // encoded responses.  See Service.java
    requestUserAgent = [NSString stringWithFormat:@"%@ %@/%@ %@%@ (gzip)",
      userAgent, libraryString, libVersionString, systemString, customString];
  }
  return requestUserAgent;
}

- (NSMutableURLRequest *)requestForURL:(NSURL *)url
                                  ETag:(NSString *)etag
                            httpMethod:(NSString *)httpMethod
                                ticket:(GTLServiceTicket *)ticket {

  // subclasses may add headers to this
  NSMutableURLRequest *request = [[[NSMutableURLRequest alloc] initWithURL:url
                                                               cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                           timeoutInterval:60] autorelease];
  NSString *requestUserAgent = self.requestUserAgent;
  [request setValue:requestUserAgent forHTTPHeaderField:@"User-Agent"];

  if ([httpMethod length] > 0) {
    [request setHTTPMethod:httpMethod];
  }

  if ([etag length] > 0) {

    // it's rather unexpected for an etagged object to be provided for a GET,
    // but we'll check for an etag anyway, similar to HttpGDataRequest.java,
    // and if present use it to request only an unchanged resource

    BOOL isDoingHTTPGet = (httpMethod == nil
               || [httpMethod caseInsensitiveCompare:@"GET"] == NSOrderedSame);

    if (isDoingHTTPGet) {

      // set the etag header, even if weak, indicating we don't want
      // another copy of the resource if it's the same as the object
      [request setValue:etag forHTTPHeaderField:@"If-None-Match"];

    } else {

      // if we're doing PUT or DELETE, set the etag header indicating
      // we only want to update the resource if our copy matches the current
      // one (unless the etag is weak and so shouldn't be a constraint at all)
      BOOL isWeakETag = [etag hasPrefix:@"W/"];

      BOOL isModifying =
        [httpMethod caseInsensitiveCompare:@"PUT"] == NSOrderedSame
        || [httpMethod caseInsensitiveCompare:@"DELETE"] == NSOrderedSame
        || [httpMethod caseInsensitiveCompare:@"PATCH"] == NSOrderedSame;

      if (isModifying && !isWeakETag) {
        [request setValue:etag forHTTPHeaderField:@"If-Match"];
      }
    }
  }

  return request;
}

- (NSMutableURLRequest *)requestForURL:(NSURL *)url
                                  ETag:(NSString *)etag
                            httpMethod:(NSString *)httpMethod {
  // this public entry point authenticates from the service object but
  // not from the auth token in the ticket
  return [self requestForURL:url ETag:etag httpMethod:httpMethod ticket:nil];
}

// objectRequestForURL returns an NSMutableURLRequest for a GTLObject
//
// the object is the object being sent to the server, or nil;
// the http method may be nil for get, or POST, PUT, DELETE

- (NSMutableURLRequest *)objectRequestForURL:(NSURL *)url
                                      object:(GTLObject *)object
                                        ETag:(NSString *)etag
                                  httpMethod:(NSString *)httpMethod
                                      isREST:(BOOL)isREST
                           additionalHeaders:(NSDictionary *)additionalHeaders
                                      ticket:(GTLServiceTicket *)ticket {
  if (object) {
    // if the object being sent has an etag, add it to the request header to
    // avoid retrieving a duplicate or to avoid writing over an updated
    // version of the resource on the server
    //
    // Typically, delete requests will provide an explicit ETag parameter, and
    // other requests will have the ETag carried inside the object being updated
    if (etag == nil) {
      SEL selEtag = @selector(ETag);
      if ([object respondsToSelector:selEtag]) {
        etag = [object performSelector:selEtag];
      }
    }
  }

  NSMutableURLRequest *request = [self requestForURL:url
                                                ETag:etag
                                          httpMethod:httpMethod
                                              ticket:ticket];
  NSString *acceptValue;
  NSString *contentTypeValue;
  if (isREST) {
    acceptValue = @"application/json";
    contentTypeValue = @"application/json; charset=utf-8";
  } else {
    acceptValue = @"application/json-rpc";
    contentTypeValue = @"application/json-rpc; charset=utf-8";
  }
  [request setValue:acceptValue forHTTPHeaderField:@"Accept"];
  [request setValue:contentTypeValue forHTTPHeaderField:@"Content-Type"];

  [request setValue:@"no-cache" forHTTPHeaderField:@"Cache-Control"];

  // Add the additional http headers from the service, and then from the query
  NSDictionary *headers = self.additionalHTTPHeaders;
  for (NSString *key in headers) {
    NSString *value = [headers valueForKey:key];
    [request setValue:value forHTTPHeaderField:key];
  }

  headers = additionalHeaders;
  for (NSString *key in headers) {
    NSString *value = [headers valueForKey:key];
    [request setValue:value forHTTPHeaderField:key];
  }

  return request;
}

#pragma mark -

// common fetch starting method

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)targetURL
                             objectClass:(Class)objectClass
                              bodyObject:(GTLObject *)bodyObject
                              dataToPost:(NSData *)dataToPost
                                    ETag:(NSString *)etag
                              httpMethod:(NSString *)httpMethod
                            mayAuthorize:(BOOL)mayAuthorize
                                  isREST:(BOOL)isREST
                                delegate:(id)delegate
                       didFinishSelector:(SEL)finishedSelector
                       completionHandler:(id)completionHandler // GTLServiceCompletionHandler
                          executingQuery:(id<GTLQueryProtocol>)query
                                  ticket:(GTLServiceTicket *)ticket {

  GTMAssertSelectorNilOrImplementedWithArgs(delegate, finishedSelector, @encode(GTLServiceTicket *), @encode(GTLObject *), @encode(NSError *), 0);

  // The completionHandler argument is declared as an id, not as a block
  // pointer, so this can be built with the 10.6 SDK and still run on 10.5.
  // If the argument were declared as a block pointer, the invocation for
  // fetchObjectWithURL: created in GTLService would cause an exception
  // since 10.5's NSInvocation cannot deal with encoding of block pointers.

  GTL_DEBUG_ASSERT(targetURL != nil, @"no url?");
  if (targetURL == nil) return nil;

  // we need to create a ticket unless one was created earlier (like during
  // authentication)
  if (!ticket) {
    ticket = [[[self class] ticketClass] ticketForService:self];
  }

  ticket.isREST = isREST;

  // Add any service specific query parameters.
  NSDictionary *urlQueryParameters = self.urlQueryParameters;
  if ([urlQueryParameters count] > 0) {
    targetURL = [GTLUtilities URLWithString:[targetURL absoluteString]
                            queryParameters:urlQueryParameters];
  }

  // If this is REST and there is a developer key, add it onto the url. RPC
  // adds the key into the payload, not on the url.
  NSString *apiKey = self.APIKey;
  if (isREST && [apiKey length] > 0) {
    NSString *const kDeveloperAPIQueryParamKey = @"key";
    NSDictionary *queryParameters;
    queryParameters = [NSDictionary dictionaryWithObject:apiKey
                                                  forKey:kDeveloperAPIQueryParamKey];
    targetURL = [GTLUtilities URLWithString:[targetURL absoluteString]
                            queryParameters:queryParameters];
  }

  NSDictionary *additionalHeaders = query.additionalHTTPHeaders;

  NSMutableURLRequest *request = [self objectRequestForURL:targetURL
                                                    object:bodyObject
                                                      ETag:etag
                                                httpMethod:httpMethod
                                                    isREST:isREST
                                         additionalHeaders:additionalHeaders
                                                    ticket:ticket];

  GTMAssertSelectorNilOrImplementedWithArgs(delegate, ticket.uploadProgressSelector,
      @encode(GTLServiceTicket *), @encode(unsigned long long),
      @encode(unsigned long long), 0);
  GTMAssertSelectorNilOrImplementedWithArgs(delegate, ticket.retrySelector,
      @encode(GTLServiceTicket *), @encode(BOOL), @encode(NSError *), 0);

  SEL finishedSel = @selector(objectFetcher:finishedWithData:error:);

  ticket.postedObject = bodyObject;

  ticket.executingQuery = query;
  if (ticket.originalQuery == nil) {
    ticket.originalQuery = query;
  }

  GTMHTTPFetcherService *fetcherService = self.fetcherService;
  GTMHTTPFetcher *fetcher;

  GTLUploadParameters *uploadParams = query.uploadParameters;
  if (uploadParams == nil) {
    // Not uploading a file with this request
    fetcher = [fetcherService fetcherWithRequest:request];
  } else {
    fetcher = [self uploadFetcherWithRequest:request
                              fetcherService:fetcherService
                                      params:uploadParams];
  }

  if (finishedSelector) {
    // if we don't have a method name, default to the finished selector as
    // a useful fetcher log comment
    fetcher.comment = NSStringFromSelector(finishedSelector);
  }

  // allow the user to specify static app-wide cookies for fetching
  NSInteger cookieStorageMethod = [self cookieStorageMethod];
  if (cookieStorageMethod >= 0) {
    fetcher.cookieStorageMethod = cookieStorageMethod;
  }

  if (!mayAuthorize) {
    fetcher.authorizer = nil;
  }

  // copy the ticket's retry settings into the fetcher
  fetcher.retryEnabled = ticket.retryEnabled;
  fetcher.maxRetryInterval = ticket.maxRetryInterval;

  BOOL shouldExamineRetries;
#if NS_BLOCKS_AVAILABLE
  shouldExamineRetries = (ticket.retrySelector != nil
                          || ticket.retryBlock != nil);
#else
  shouldExamineRetries = (ticket.retrySelector != nil);
#endif
  if (shouldExamineRetries) {
    [fetcher setRetrySelector:@selector(objectFetcher:willRetry:forError:)];
  }

  // remember the object fetcher in the ticket
  ticket.objectFetcher = fetcher;

  // add parameters used by the callbacks

  [fetcher setProperty:objectClass forKey:kFetcherObjectClassKey];

  [fetcher setProperty:delegate forKey:kFetcherDelegateKey];

  [fetcher setProperty:NSStringFromSelector(finishedSelector)
                forKey:kFetcherFinishedSelectorKey];

  [fetcher setProperty:ticket
                forKey:kFetcherTicketKey];

#if NS_BLOCKS_AVAILABLE
  // copy the completion handler block to the heap; this does nothing if the
  // block is already on the heap
  completionHandler = [[completionHandler copy] autorelease];
  [fetcher setProperty:completionHandler
                forKey:kFetcherCompletionHandlerKey];
#endif

  // set the upload data
  fetcher.postData = dataToPost;

  // failed fetches call the failure selector, which will delete the ticket
  BOOL didFetch = [fetcher beginFetchWithDelegate:self
                                didFinishSelector:finishedSel];

  // If something weird happens and the networking callbacks have been called
  // already synchronously, we don't want to return the ticket since the caller
  // will never know when to stop retaining it, so we'll make sure the
  // success/failure callbacks have not yet been called by checking the
  // ticket
  if (!didFetch || ticket.hasCalledCallback) {
    fetcher.properties = nil;
    return nil;
  }

  return ticket;
}

- (GTMHTTPUploadFetcher *)uploadFetcherWithRequest:(NSURLRequest *)request
                                    fetcherService:(GTMHTTPFetcherService *)fetcherService
                                            params:(GTLUploadParameters *)uploadParams {
  // Hang on to the user's requested chunk size, and ensure it's not tiny
  NSUInteger uploadChunkSize = [self serviceUploadChunkSize];
  if (uploadChunkSize < kMinimumUploadChunkSize) {
    uploadChunkSize = kMinimumUploadChunkSize;
  }

#ifdef GTL_TARGET_NAMESPACE
  // Prepend the class name prefix
  Class uploadClass = NSClassFromString(@GTL_TARGET_NAMESPACE_STRING
                                        "_GTMHTTPUploadFetcher");
#else
  Class uploadClass = NSClassFromString(@"GTMHTTPUploadFetcher");
#endif
  GTL_ASSERT(uploadClass != nil, @"GTMHTTPUploadFetcher needed");

  NSString *uploadMIMEType = uploadParams.MIMEType;
  NSData *uploadData = uploadParams.data;
  NSFileHandle *uploadFileHandle = uploadParams.fileHandle;
  NSURL *uploadLocationURL = uploadParams.uploadLocationURL;

  GTMHTTPUploadFetcher *fetcher;
  if (uploadData) {
    fetcher = [uploadClass uploadFetcherWithRequest:request
                                         uploadData:uploadData
                                     uploadMIMEType:uploadMIMEType
                                          chunkSize:uploadChunkSize
                                     fetcherService:fetcherService];
  } else if (uploadLocationURL) {
    GTL_DEBUG_ASSERT(uploadFileHandle != nil,
                     @"Resume requires a file handle");
    fetcher = [uploadClass uploadFetcherWithLocation:uploadLocationURL
                                    uploadFileHandle:uploadFileHandle
                                      uploadMIMEType:uploadMIMEType
                                           chunkSize:uploadChunkSize
                                      fetcherService:fetcherService];
  } else {
    fetcher = [uploadClass uploadFetcherWithRequest:request
                                   uploadFileHandle:uploadFileHandle
                                     uploadMIMEType:uploadMIMEType
                                          chunkSize:uploadChunkSize
                                     fetcherService:fetcherService];
  }

  NSString *slug = [uploadParams slug];
  if ([slug length] > 0) {
    [[fetcher mutableRequest] setValue:slug forHTTPHeaderField:@"Slug"];
  }
  return fetcher;
}

#pragma mark -

// RPC fetch methods

- (NSDictionary *)rpcPayloadForMethodNamed:(NSString *)methodName
                                parameters:(NSDictionary *)parameters
                                bodyObject:(GTLObject *)bodyObject
                                 requestID:(NSString *)requestID {
  GTL_DEBUG_ASSERT([requestID length] > 0, @"Got an empty request id");

  // First, merge the developer key and bodyObject into the parameters.

  NSString *apiKey = self.APIKey;
  NSUInteger apiKeyLen = [apiKey length];

  NSString *const kDeveloperAPIParamKey = @"key";
  NSString *const kBodyObjectParamKey = @"resource";

  NSDictionary *finalParams;
  if ((apiKeyLen == 0) && (bodyObject == nil)) {
    // Nothing needs to be added, just send the dict along.
    finalParams = parameters;
  } else {
    NSMutableDictionary *worker = [NSMutableDictionary dictionary];
    if ([parameters count] > 0) {
      [worker addEntriesFromDictionary:parameters];
    }
    if ((apiKeyLen > 0)
        && ([worker objectForKey:kDeveloperAPIParamKey] == nil)) {
      [worker setObject:apiKey forKey:kDeveloperAPIParamKey];
    }
    if (bodyObject != nil) {
      GTL_DEBUG_ASSERT([parameters objectForKey:kBodyObjectParamKey] == nil,
                       @"There was already something under the 'data' key?!");
      NSMutableDictionary *json = [bodyObject JSON];
      if (json != nil) {
        [worker setObject:json forKey:kBodyObjectParamKey];
      }
    }
    finalParams = worker;
  }

  // Now, build up the full dictionary for the JSON-RPC (this is the body of
  // the HTTP PUT).

  // Spec calls for the jsonrpc entry.  Google doesn't require it, but include
  // it so the code can work with other servers.
  NSMutableDictionary *rpcPayload = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                     @"2.0", @"jsonrpc",
                                     methodName, @"method",
                                     requestID, @"id",
                                     nil];

  // Google extension, provide the version of the api.
  NSString *apiVersion = self.apiVersion;
  if ([apiVersion length] > 0) {
    [rpcPayload setObject:apiVersion forKey:@"apiVersion"];
  }

  if ([finalParams count] > 0) {
    [rpcPayload setObject:finalParams forKey:@"params"];
  }

  return rpcPayload;
}

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                     objectClass:(Class)objectClass
                                      parameters:(NSDictionary *)parameters
                                      bodyObject:(GTLObject *)bodyObject
                                       requestID:(NSString *)requestID
                              urlQueryParameters:(NSDictionary *)urlQueryParameters
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector
                               completionHandler:(id)completionHandler // GTLServiceCompletionHandler
                                  executingQuery:(id<GTLQueryProtocol>)executingQuery
                                          ticket:(GTLServiceTicket *)ticket {
  GTL_DEBUG_ASSERT([methodName length] > 0, @"Got an empty method name");
  if ([methodName length] == 0) return nil;

  // If we didn't get a requestID, assign one (call came from one of the public
  // calls that doesn't take a GTLQuery object).
  if (requestID == nil) {
    requestID = [GTLQuery nextRequestID];
  }

  NSData *dataToPost = nil;
  GTLUploadParameters *uploadParameters = executingQuery.uploadParameters;
  BOOL shouldSendBody = !uploadParameters.shouldSendUploadOnly;
  if (shouldSendBody) {
    NSDictionary *rpcPayload = [self rpcPayloadForMethodNamed:methodName
                                                   parameters:parameters
                                                   bodyObject:bodyObject
                                                    requestID:requestID];

    NSError *error = nil;
    dataToPost = [GTLJSONParser dataWithObject:rpcPayload
                                 humanReadable:NO
                                         error:&error];
    if (dataToPost == nil) {
      // There is the chance something went into parameters that wasn't valid.
      GTL_DEBUG_LOG(@"JSON generation error: %@", error);
      return nil;
    }
  }

  BOOL isUploading = (uploadParameters != nil);
  NSURL *rpcURL = (isUploading ? self.rpcUploadURL : self.rpcURL);

  if ([urlQueryParameters count] > 0) {
    rpcURL = [GTLUtilities URLWithString:[rpcURL absoluteString]
                         queryParameters:urlQueryParameters];
  }

  BOOL mayAuthorize = (executingQuery ?
                       !executingQuery.shouldSkipAuthorization : YES);

  GTLServiceTicket *resultTicket = [self fetchObjectWithURL:rpcURL
                                                objectClass:objectClass
                                                 bodyObject:bodyObject
                                                 dataToPost:dataToPost
                                                       ETag:nil
                                                 httpMethod:@"POST"
                                               mayAuthorize:mayAuthorize
                                                     isREST:NO
                                                   delegate:delegate
                                          didFinishSelector:finishedSelector
                                          completionHandler:completionHandler
                                             executingQuery:executingQuery
                                                     ticket:ticket];

  // Set the fetcher log comment to default to the method name
  NSUInteger pageNumber = resultTicket.pagesFetchedCounter;
  if (pageNumber == 0) {
    resultTicket.objectFetcher.comment = methodName;
  } else {
    // Also put the page number in the log comment
    [resultTicket.objectFetcher setCommentWithFormat:@"%@ (page %lu)",
     methodName, (unsigned long) (pageNumber + 1)];
  }

  return resultTicket;
}

- (GTLServiceTicket *)executeBatchQuery:(GTLBatchQuery *)batch
                               delegate:(id)delegate
                      didFinishSelector:(SEL)finishedSelector
                      completionHandler:(id)completionHandler // GTLServiceCompletionHandler
                                 ticket:(GTLServiceTicket *)ticket {
  GTLBatchQuery *batchCopy = [[batch copy] autorelease];
  NSArray *queries = batchCopy.queries;
  NSUInteger numberOfQueries = [queries count];
  if (numberOfQueries == 0) return nil;

  // Build up the array of RPC calls.
  NSMutableArray *rpcPayloads = [NSMutableArray arrayWithCapacity:numberOfQueries];
  NSMutableArray *requestIDs = [NSMutableSet setWithCapacity:numberOfQueries];
  for (GTLQuery *query in queries) {
    NSString *methodName = query.methodName;
    NSDictionary *parameters = query.JSON;
    GTLObject *bodyObject = query.bodyObject;
    NSString *requestID = query.requestID;

    if ([methodName length] == 0 || [requestID length] == 0) {
      GTL_DEBUG_ASSERT(0, @"Invalid query - id:%@ method:%@",
                       requestID, methodName);
      return nil;
    }

    GTL_DEBUG_ASSERT(query.additionalHTTPHeaders == nil,
                     @"additionalHTTPHeaders disallowed on queries added to a batch - query %@ (%@)",
                     requestID, methodName);

    GTL_DEBUG_ASSERT(query.uploadParameters == nil,
                     @"uploadParameters disallowed on queries added to a batch - query %@ (%@)",
                     requestID, methodName);

    NSDictionary *rpcPayload = [self rpcPayloadForMethodNamed:methodName
                                                   parameters:parameters
                                                   bodyObject:bodyObject
                                                    requestID:requestID];
    [rpcPayloads addObject:rpcPayload];

    if ([requestIDs containsObject:requestID]) {
      GTL_DEBUG_LOG(@"Duplicate request id in batch: %@", requestID);
      return nil;
    }
    [requestIDs addObject:requestID];
  }

  NSError *error = nil;
  NSData *dataToPost = nil;
  dataToPost = [GTLJSONParser dataWithObject:rpcPayloads
                               humanReadable:NO
                                       error:&error];
  if (dataToPost == nil) {
    // There is the chance something went into parameters that wasn't valid.
    GTL_DEBUG_LOG(@"JSON generation error: %@", error);
    return nil;
  }

  BOOL mayAuthorize = (batchCopy ? !batchCopy.shouldSkipAuthorization : YES);

  // urlQueryParameters on the queries are currently unsupport during a batch
  // as it's not clear how to map them.

  NSURL *rpcURL = self.rpcURL;
  GTLServiceTicket *resultTicket = [self fetchObjectWithURL:rpcURL
                                                objectClass:[GTLBatchResult class]
                                                 bodyObject:nil
                                                 dataToPost:dataToPost
                                                       ETag:nil
                                                 httpMethod:@"POST"
                                               mayAuthorize:mayAuthorize
                                                     isREST:NO
                                                   delegate:delegate
                                          didFinishSelector:finishedSelector
                                          completionHandler:completionHandler
                                             executingQuery:batch
                                                     ticket:ticket];

#if !STRIP_GTM_FETCH_LOGGING
  // Set the fetcher log comment
  //
  // Because this has expensive set operations, it's conditionally
  // compiled in
  NSArray *methodNames = [queries valueForKey:@"methodName"];
  methodNames = [[NSSet setWithArray:methodNames] allObjects]; // de-dupe
  NSString *methodsStr = [methodNames componentsJoinedByString:@", "];

  NSUInteger pageNumber = ticket.pagesFetchedCounter;
  NSString *pageStr = @"";
  if (pageNumber > 0) {
    pageStr = [NSString stringWithFormat:@"page %lu, ",
               (unsigned long) pageNumber + 1];
  }
  [resultTicket.objectFetcher setCommentWithFormat:@"batch: %@ (%@%lu queries)",
   methodsStr, pageStr, (unsigned long) numberOfQueries];
#endif

  return resultTicket;
}


#pragma mark -

// REST fetch methods

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)targetURL
                             objectClass:(Class)objectClass
                              bodyObject:(GTLObject *)bodyObject
                                    ETag:(NSString *)etag
                              httpMethod:(NSString *)httpMethod
                            mayAuthorize:(BOOL)mayAuthorize
                                delegate:(id)delegate
                       didFinishSelector:(SEL)finishedSelector
                       completionHandler:(id)completionHandler // GTLServiceCompletionHandler
                                  ticket:(GTLServiceTicket *)ticket {
  // if no URL was supplied, treat this as if the fetch failed (below)
  // and immediately return a nil ticket, skipping the callbacks
  //
  // this might be considered normal (say, updating a read-only entry
  // that lacks an edit link) though higher-level calls may assert or
  // return errors depending on the specific usage
  if (targetURL == nil) return nil;

  NSData *dataToPost = nil;
  if (bodyObject != nil) {
    NSError *error = nil;

    NSDictionary *whatToSend;
    NSDictionary *json = bodyObject.JSON;
    if (isRESTDataWrapperRequired_) {
      // create the top-level "data" object
      NSDictionary *dataDict = [NSDictionary dictionaryWithObject:json
                                                           forKey:@"data"];
      whatToSend = dataDict;
    } else {
      whatToSend = json;
    }
    dataToPost = [GTLJSONParser dataWithObject:whatToSend
                                 humanReadable:NO
                                         error:&error];
    if (dataToPost == nil) {
      GTL_DEBUG_LOG(@"JSON generation error: %@", error);
    }
  }

  return [self fetchObjectWithURL:targetURL
                      objectClass:objectClass
                       bodyObject:bodyObject
                       dataToPost:dataToPost
                             ETag:etag
                       httpMethod:httpMethod
                     mayAuthorize:mayAuthorize
                           isREST:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:completionHandler
                   executingQuery:nil
                           ticket:ticket];
}

- (void)invokeProgressCallbackForTicket:(GTLServiceTicket *)ticket
                         deliveredBytes:(unsigned long long)numReadSoFar
                             totalBytes:(unsigned long long)total {

  SEL progressSelector = [ticket uploadProgressSelector];
  if (progressSelector) {

    GTMHTTPFetcher *fetcher = ticket.objectFetcher;
    id delegate = [fetcher propertyForKey:kFetcherDelegateKey];

    NSMethodSignature *signature = [delegate methodSignatureForSelector:progressSelector];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];

    [invocation setSelector:progressSelector];
    [invocation setTarget:delegate];
    [invocation setArgument:&ticket atIndex:2];
    [invocation setArgument:&numReadSoFar atIndex:3];
    [invocation setArgument:&total atIndex:4];
    [invocation invoke];
  }

#if NS_BLOCKS_AVAILABLE
  GTLServiceUploadProgressBlock block = ticket.uploadProgressBlock;
  if (block) {
    block(ticket, numReadSoFar, total);
  }
#endif
}

// sentData callback from fetcher
- (void)objectFetcher:(GTMHTTPFetcher *)fetcher
         didSendBytes:(NSInteger)bytesSent
       totalBytesSent:(NSInteger)totalBytesSent
totalBytesExpectedToSend:(NSInteger)totalBytesExpected {

  GTLServiceTicket *ticket = [fetcher propertyForKey:kFetcherTicketKey];

  [self invokeProgressCallbackForTicket:ticket
                         deliveredBytes:(unsigned long long)totalBytesSent
                             totalBytes:(unsigned long long)totalBytesExpected];
}

- (void)objectFetcher:(GTMHTTPFetcher *)fetcher finishedWithData:(NSData *)data error:(NSError *)error {
  // we now have the JSON data for an object, or an error
  if (error == nil) {
    if ([data length] > 0) {
      [self prepareToParseObjectForFetcher:fetcher];
    } else {
      // no data (such as when deleting)
      [self handleParsedObjectForFetcher:fetcher];
    }
  } else {
    // There was an error from the fetch
    NSInteger status = [error code];
    if (status >= 300) {
      // Return the HTTP error status code along with a more descriptive error
      // from within the HTTP response payload.
      NSData *responseData = fetcher.downloadedData;
      if ([responseData length] > 0) {
        NSDictionary *responseHeaders = fetcher.responseHeaders;
        NSString *contentType = [responseHeaders objectForKey:@"Content-Type"];

        if ([data length] > 0) {
          if ([contentType hasPrefix:@"application/json"]) {
            NSError *parseError = nil;
            NSMutableDictionary *jsonWrapper = [GTLJSONParser objectWithData:data
                                                                       error:&parseError];
            if (parseError) {
              // We could not parse the JSON payload
              error = parseError;
            } else {
              // Convert the JSON error payload into a structured error
              NSMutableDictionary *errorJSON = [jsonWrapper valueForKey:@"error"];
              GTLErrorObject *errorObject = [GTLErrorObject objectWithJSON:errorJSON];
              error = [errorObject foundationError];
            }
          } else {
            // No structured JSON error was available; make a plaintext server
            // error response visible in the error object.
            NSString *reasonStr = [[[NSString alloc] initWithData:data
                                                         encoding:NSUTF8StringEncoding] autorelease];
            NSDictionary *userInfo = [NSDictionary dictionaryWithObject:reasonStr
                                                                 forKey:NSLocalizedFailureReasonErrorKey];
            error = [NSError errorWithDomain:kGTMHTTPFetcherStatusDomain
                                        code:status
                                    userInfo:userInfo];
          }
        } else {
          // Response data length is zero; we'll settle for returning the
          // fetcher's error.
        }
      }
    }

    // store the error, call the callbacks, and bail
    [fetcher setProperty:error
                  forKey:kFetcherFetchErrorKey];

    [self handleParsedObjectForFetcher:fetcher];
  }
}

// Three methods handle parsing of the fetched JSON data:
//   - prepareToParse posts a start notification and then spawns off parsing
//     on the operation queue (if there's an operation queue)
//   - parseObject does the parsing of the JSON string
//   - handleParsedObject posts the stop notification and calls the callback
//     with the parsed object or an error
//
// The middle method may run on a separate thread.

- (void)prepareToParseObjectForFetcher:(GTMHTTPFetcher *)fetcher {
  // save the current thread into the fetcher, since we'll handle additional
  // fetches and callbacks on this thread
  [fetcher setProperty:[NSThread currentThread]
                forKey:kFetcherCallbackThreadKey];

  // copy the run loop modes, if any, so we don't need to access them
  // from the parsing thread
  [fetcher setProperty:[[[self runLoopModes] copy] autorelease]
                forKey:kFetcherCallbackRunLoopModesKey];

  // we post parsing notifications now to ensure they're on caller's
  // original thread
  GTLServiceTicket *ticket = [fetcher propertyForKey:kFetcherTicketKey];
  NSNotificationCenter *defaultNC = [NSNotificationCenter defaultCenter];
  [defaultNC postNotificationName:kGTLServiceTicketParsingStartedNotification
                           object:ticket];
  [fetcher setProperty:@"1"
                forKey:kFetcherParsingNotificationKey];

  id<GTLQueryProtocol> executingQuery = ticket.executingQuery;
  if ([executingQuery isBatchQuery]) {
    // build a dictionary of expected classes for the batch responses
    GTLBatchQuery *batchQuery = executingQuery;
    NSArray *queries = batchQuery.queries;
    NSDictionary *batchClassMap = [NSMutableDictionary dictionaryWithCapacity:[queries count]];
    for (GTLQuery *query in queries) {
      [batchClassMap setValue:query.expectedObjectClass
                       forKey:query.requestID];
    }
    [fetcher setProperty:batchClassMap
                  forKey:kFetcherBatchClassMapKey];
  }

  // if there's an operation queue, then use that to schedule parsing on another
  // thread
  SEL parseSel = @selector(parseObjectFromDataOfFetcher:);
  NSOperationQueue *queue = self.parseQueue;
  if (queue) {
    NSInvocationOperation *op;
    op = [[[NSInvocationOperation alloc] initWithTarget:self
                                               selector:parseSel
                                                 object:fetcher] autorelease];
    ticket.parseOperation = op;
    [queue addOperation:op];
    // the fetcher now belongs to the parsing thread
  } else {
    // parse on the current thread, on Mac OS X 10.4 through 10.5.7
    // or when the project defines GTL_SKIP_PARSE_THREADING
    [self performSelector:parseSel
               withObject:fetcher];
  }
}

- (void)parseObjectFromDataOfFetcher:(GTMHTTPFetcher *)fetcher {
  // This method runs in a separate thread

  // Generally protect the fetcher properties, since canceling a ticket would
  // release the fetcher properties dictionary
  NSMutableDictionary *properties = [[fetcher.properties retain] autorelease];

  // The callback thread is retaining the fetcher, so the fetcher shouldn't keep
  // retaining the callback thread
  NSThread *callbackThread = [properties valueForKey:kFetcherCallbackThreadKey];
  [[callbackThread retain] autorelease];
  [properties removeObjectForKey:kFetcherCallbackThreadKey];

  GTLServiceTicket *ticket = [properties valueForKey:kFetcherTicketKey];
  [[ticket retain] autorelease];

  NSDictionary *responseHeaders = fetcher.responseHeaders;
  NSString *contentType = [responseHeaders objectForKey:@"Content-Type"];
  NSData *data = fetcher.downloadedData;

  NSOperation *parseOperation = ticket.parseOperation;

  GTL_DEBUG_ASSERT([contentType hasPrefix:@"application/json"],
                   @"Got unexpected content type '%@'", contentType);
  if ([contentType hasPrefix:@"application/json"] && [data length] > 0) {
#if GTL_LOG_PERFORMANCE
    NSTimeInterval secs1, secs2;
    secs1 = [NSDate timeIntervalSinceReferenceDate];
#endif

    NSError *parseError = nil;
    NSMutableDictionary *jsonWrapper = [GTLJSONParser objectWithData:data
                                                               error:&parseError];
    if ([parseOperation isCancelled]) return;

    if (parseError != nil) {
      [properties setValue:parseError forKey:kFetcherFetchErrorKey];
    } else {
      NSMutableDictionary *json;
      NSDictionary *batchClassMap = nil;

      // In theory, just checking for "application/json-rpc" vs
      // "application/json" would work. But the JSON-RPC spec allows for
      // "application/json" also so we have to carry a flag all the way in
      // saying which type of result to expect and process as.
      BOOL isREST = ticket.isREST;
      if (isREST) {
        if (isRESTDataWrapperRequired_) {
          json = [jsonWrapper valueForKey:@"data"];
        } else {
          json = jsonWrapper;
        }
      } else {
        batchClassMap = [properties valueForKey:kFetcherBatchClassMapKey];
        if (batchClassMap) {
          // A batch gets the whole array as it's json.
          json = jsonWrapper;
        } else {
          json = [jsonWrapper valueForKey:@"result"];
        }
      }

      if (json != nil) {
        Class defaultClass = [properties valueForKey:kFetcherObjectClassKey];
        NSDictionary *surrogates = ticket.surrogates;

        GTLObject *parsedObject = [GTLObject objectForJSON:json
                                              defaultClass:defaultClass
                                                surrogates:surrogates
                                             batchClassMap:batchClassMap];

        [properties setValue:parsedObject forKey:kFetcherParsedObjectKey];
      } else if (!isREST) {
        NSMutableDictionary *errorJSON = [jsonWrapper valueForKey:@"error"];
        GTL_DEBUG_ASSERT(errorJSON != nil, @"no result or error in response:\n%@",
                         jsonWrapper);
        GTLErrorObject *errorObject = [GTLErrorObject objectWithJSON:errorJSON];
        NSError *error = [errorObject foundationError];

        // Store the error and let it go to the callback
        [properties setValue:error
                      forKey:kFetcherFetchErrorKey];
      }
    }

#if GTL_LOG_PERFORMANCE
    secs2 = [NSDate timeIntervalSinceReferenceDate];
    NSLog(@"allocation of %@ took %f seconds", objectClass, secs2 - secs1);
#endif
  }

  if ([parseOperation isCancelled]) return;

  SEL parseDoneSel = @selector(handleParsedObjectForFetcher:);
  NSArray *runLoopModes = [properties valueForKey:kFetcherCallbackRunLoopModesKey];
  // If this callback was enqueued, then the fetcher has already released
  // its delegateQueue.  We'll use our own delegateQueue to determine how to
  // invoke the callbacks.
  NSOperationQueue *delegateQueue = self.delegateQueue;
  if (delegateQueue) {
    NSInvocationOperation *op;
    op = [[[NSInvocationOperation alloc] initWithTarget:self
                                               selector:parseDoneSel
                                                 object:fetcher] autorelease];
    [delegateQueue addOperation:op];
  } else if (runLoopModes) {
    [self performSelector:parseDoneSel
                 onThread:callbackThread
               withObject:fetcher
            waitUntilDone:NO
                    modes:runLoopModes];
  } else {
    // Defaults to common modes
    [self performSelector:parseDoneSel
                 onThread:callbackThread
               withObject:fetcher
            waitUntilDone:NO];
  }
  // the fetcher now belongs to the callback thread
}

- (void)handleParsedObjectForFetcher:(GTMHTTPFetcher *)fetcher {
  // After parsing is complete, this is invoked on the thread that the
  // fetch was performed on
  //
  // There may not be an object due to a fetch or parsing error

  GTLServiceTicket *ticket = [fetcher propertyForKey:kFetcherTicketKey];
  ticket.parseOperation = nil;

  // unpack the callback parameters
  id delegate = [fetcher propertyForKey:kFetcherDelegateKey];
  NSString *selString = [fetcher propertyForKey:kFetcherFinishedSelectorKey];
  SEL finishedSelector = NSSelectorFromString(selString);

#if NS_BLOCKS_AVAILABLE
  GTLServiceCompletionHandler completionHandler;
  completionHandler = [fetcher propertyForKey:kFetcherCompletionHandlerKey];
#else
  id completionHandler = nil;
#endif

  GTLObject *object = [fetcher propertyForKey:kFetcherParsedObjectKey];
  NSError *error = [fetcher propertyForKey:kFetcherFetchErrorKey];

  GTLQuery *executingQuery = (GTLQuery *)ticket.executingQuery;

  BOOL shouldFetchNextPages = ticket.shouldFetchNextPages;
  GTLObject *previousObject = ticket.fetchedObject;

  if (shouldFetchNextPages
      && (previousObject != nil)
      && (object != nil)) {
    // Accumulate new results
    object = [self mergedNewResultObject:object
                         oldResultObject:previousObject
                                forQuery:executingQuery];
  }

  ticket.fetchedObject = object;
  ticket.fetchError = error;

  if ([fetcher propertyForKey:kFetcherParsingNotificationKey] != nil) {
    // we want to always balance the start and stop notifications
    NSNotificationCenter *defaultNC = [NSNotificationCenter defaultCenter];
    [defaultNC postNotificationName:kGTLServiceTicketParsingStoppedNotification
                             object:ticket];
  }

  BOOL shouldCallCallbacks = YES;

  // Use the nextPageToken to fetch any later pages for non-batch queries
  //
  // This assumes a pagination model where objects have entries in an "items"
  // field and a "nextPageToken" field, and queries support a "pageToken"
  // parameter.
  if (ticket.shouldFetchNextPages) {
    // Determine if we should fetch more pages of results

    GTLQuery *nextPageQuery = [self nextPageQueryForQuery:executingQuery
                                                   result:object
                                                   ticket:ticket];
    if (nextPageQuery) {
      BOOL isFetchingMore = [self fetchNextPageWithQuery:nextPageQuery
                                                delegate:delegate
                                     didFinishedSelector:finishedSelector
                                       completionHandler:completionHandler
                                                  ticket:ticket];
      if (isFetchingMore) {
        shouldCallCallbacks = NO;
      }
    } else {
      // No more page tokens are present
#if DEBUG && !GTL_SKIP_PAGES_WARNING
      // Each next page followed to accumulate all pages of a feed takes up to
      // a few seconds.  When multiple pages are being fetched, that
      // usually indicates that a larger page size (that is, more items per
      // feed fetched) should be requested.
      //
      // To avoid fetching many pages, set query.maxResults so the feed
      // requested is large enough to rarely need to follow next links.
      NSUInteger pageCount = ticket.pagesFetchedCounter;
      if (pageCount > 2) {
        NSString *queryLabel = [executingQuery isBatchQuery] ?
          @"batch query" : executingQuery.methodName;
        NSLog(@"Executing %@ required fetching %u pages; use a query with a"
              @" larger maxResults for faster results",
              queryLabel, (unsigned int) pageCount);
      }
#endif
    }
  }

  // We no longer care about the queries for page 2 or later, so for the client
  // inspecting the ticket in the callback, the executing query should be
  // the original one
  ticket.executingQuery = ticket.originalQuery;

  if (shouldCallCallbacks) {
    // First, call query-specific callback blocks.  We do this before the
    // fetch callback to let applications do any final clean-up (or update
    // their UI) in the fetch callback.
    GTLQuery *originalQuery = (GTLQuery *)ticket.originalQuery;
#if NS_BLOCKS_AVAILABLE
    if (![originalQuery isBatchQuery]) {
      // Single query
      GTLServiceCompletionHandler completionBlock = originalQuery.completionBlock;
      if (completionBlock) {
        completionBlock(ticket, object, error);
      }
    } else {
      // Batch query
      //
      // We'll step through the queries of the original batch, not of the
      // batch result
      GTLBatchQuery *batchQuery = (GTLBatchQuery *)originalQuery;
      GTLBatchResult *batchResult = (GTLBatchResult *)object;
      NSDictionary *successes = batchResult.successes;
      NSDictionary *failures = batchResult.failures;

      for (GTLQuery *oneQuery in batchQuery.queries) {
        GTLServiceCompletionHandler completionBlock = oneQuery.completionBlock;
        if (completionBlock) {
          // If there was no networking error, look for a query-specific
          // error or result
          GTLObject *oneResult = nil;
          NSError *oneError = error;
          if (oneError == nil) {
            NSString *requestID = [oneQuery requestID];
            GTLErrorObject *gtlError = [failures objectForKey:requestID];
            if (gtlError) {
              oneError = [gtlError foundationError];
            } else {
              oneResult = [successes objectForKey:requestID];
              if (oneResult == nil) {
                // We found neither a success nor a failure for this
                // query, unexpectedly
                GTL_DEBUG_LOG(@"GTLService: Batch result missing for request %@",
                              requestID);
                oneError = [NSError errorWithDomain:kGTLServiceErrorDomain
                                               code:kGTLErrorQueryResultMissing
                                           userInfo:nil];
              }
            }
          }
          completionBlock(ticket, oneResult, oneError);
        }
      }
    }
#endif
    // Release query callback blocks
    [originalQuery executionDidStop];

    if (finishedSelector) {
      [[self class] invokeCallback:finishedSelector
                            target:delegate
                            ticket:ticket
                            object:object
                             error:error];
    }

#if NS_BLOCKS_AVAILABLE
    if (completionHandler) {
      completionHandler(ticket, object, error);
    }
#endif
    ticket.hasCalledCallback = YES;
  }
  fetcher.properties = nil;

#if NS_BLOCKS_AVAILABLE
  // Tickets don't know when the fetch has completed, so the service will
  // release their blocks here to avoid unintended retain loops
  ticket.retryBlock = nil;
  ticket.uploadProgressBlock = nil;
#endif
}

#pragma mark -

+ (void)invokeCallback:(SEL)callbackSel
                target:(id)target
                ticket:(id)ticket
                object:(id)object
                 error:(id)error {

  // GTL fetch callbacks have no return value
  NSMethodSignature *signature = [target methodSignatureForSelector:callbackSel];
  NSInvocation *retryInvocation = [NSInvocation invocationWithMethodSignature:signature];
  [retryInvocation setSelector:callbackSel];
  [retryInvocation setTarget:target];
  [retryInvocation setArgument:&ticket atIndex:2];
  [retryInvocation setArgument:&object atIndex:3];
  [retryInvocation setArgument:&error atIndex:4];
  [retryInvocation invoke];
}

// The object fetcher may call into this retry method; this one invokes the
// selector provided by the user.
- (BOOL)objectFetcher:(GTMHTTPFetcher *)fetcher willRetry:(BOOL)willRetry forError:(NSError *)error {

  GTLServiceTicket *ticket = [fetcher propertyForKey:kFetcherTicketKey];
  SEL retrySelector = ticket.retrySelector;
  if (retrySelector) {
    id delegate = [fetcher propertyForKey:kFetcherDelegateKey];

    willRetry = [self invokeRetrySelector:retrySelector
                                 delegate:delegate
                                   ticket:ticket
                                willRetry:willRetry
                                    error:error];
  }

#if NS_BLOCKS_AVAILABLE
  BOOL (^retryBlock)(GTLServiceTicket *, BOOL, NSError *) = ticket.retryBlock;
  if (retryBlock) {
    willRetry = retryBlock(ticket, willRetry, error);
  }
#endif

  return willRetry;
}

- (BOOL)invokeRetrySelector:(SEL)retrySelector
                   delegate:(id)delegate
                     ticket:(GTLServiceTicket *)ticket
                  willRetry:(BOOL)willRetry
                      error:(NSError *)error {

  if ([delegate respondsToSelector:retrySelector]) {
    // Unlike the retry selector invocation in GTMHTTPFetcher, this invocation
    // passes the ticket rather than the fetcher as argument 2
    NSMethodSignature *signature = [delegate methodSignatureForSelector:retrySelector];
    NSInvocation *retryInvocation = [NSInvocation invocationWithMethodSignature:signature];
    [retryInvocation setSelector:retrySelector];
    [retryInvocation setTarget:delegate];
    [retryInvocation setArgument:&ticket atIndex:2]; // ticket passed
    [retryInvocation setArgument:&willRetry atIndex:3];
    [retryInvocation setArgument:&error atIndex:4];
    [retryInvocation invoke];

    [retryInvocation getReturnValue:&willRetry];
  }
  return willRetry;
}

- (BOOL)waitForTicket:(GTLServiceTicket *)ticket
              timeout:(NSTimeInterval)timeoutInSeconds
        fetchedObject:(GTLObject **)outObjectOrNil
                error:(NSError **)outErrorOrNil {

  NSDate* giveUpDate = [NSDate dateWithTimeIntervalSinceNow:timeoutInSeconds];

  // loop until the fetch completes with an object or an error,
  // or until the timeout has expired
  while (![ticket hasCalledCallback]
         && [giveUpDate timeIntervalSinceNow] > 0) {

    // run the current run loop 1/1000 of a second to give the networking
    // code a chance to work
    NSDate *stopDate = [NSDate dateWithTimeIntervalSinceNow:0.001];
    [[NSRunLoop currentRunLoop] runUntilDate:stopDate];
  }

  NSError *fetchError = ticket.fetchError;

  if (![ticket hasCalledCallback] && fetchError == nil) {
    fetchError = [NSError errorWithDomain:kGTLServiceErrorDomain
                                     code:kGTLErrorWaitTimedOut
                                 userInfo:nil];
  }

  if (outObjectOrNil) *outObjectOrNil = ticket.fetchedObject;
  if (outErrorOrNil)  *outErrorOrNil = fetchError;

  return (fetchError == nil);
}

#pragma mark -

// Given a single or batch query and its result, make a new query
// for the next pages, if any.  Returns nil if there's no additional
// query to make.
//
// This method calls itself recursively to make the individual next page
// queries for a batch query.
- (id <GTLQueryProtocol>)nextPageQueryForQuery:(GTLQuery *)query
                                        result:(GTLObject *)object
                                        ticket:(GTLServiceTicket *)ticket {
  if (!query.isBatchQuery) {
    // This is a single query

    // Determine if we should fetch more pages of results
    GTLQuery *nextPageQuery = nil;
    NSString *nextPageToken = nil;
    NSNumber *nextStartIndex = nil;

    if ([object respondsToSelector:@selector(nextPageToken)]
        && [query respondsToSelector:@selector(pageToken)]) {
      nextPageToken = [object performSelector:@selector(nextPageToken)];
    }

    if ([object respondsToSelector:@selector(nextStartIndex)]
        && [query respondsToSelector:@selector(startIndex)]) {
      nextStartIndex = [object performSelector:@selector(nextStartIndex)];
    }

    if (nextPageToken || nextStartIndex) {
      // Make a query for the next page, preserving the request ID
      nextPageQuery = [[query copy] autorelease];
      nextPageQuery.requestID = query.requestID;

      if (nextPageToken) {
        [nextPageQuery performSelector:@selector(setPageToken:)
                            withObject:nextPageToken];
      } else {
        // Use KVC to unwrap the scalar type instead of converting the
        // NSNumber to an integer and using NSInvocation
        [nextPageQuery setValue:nextStartIndex
                         forKey:@"startIndex"];
      }
    }
    return nextPageQuery;
  } else {
    // This is a batch query
    //
    // Check if there's a next page to fetch for any of the success
    // results by invoking this method recursively on each of those results
    GTLBatchResult *batchResult = (GTLBatchResult *)object;
    GTLBatchQuery *nextPageBatchQuery = nil;
    NSDictionary *successes = batchResult.successes;

    for (NSString *requestID in successes) {
      GTLObject *singleObject = [successes objectForKey:requestID];
      GTLQuery *singleQuery = [ticket queryForRequestID:requestID];

      GTLQuery *newQuery = [self nextPageQueryForQuery:singleQuery
                                                result:singleObject
                                                ticket:ticket];
      if (newQuery) {
        // There is another query to fetch
        if (nextPageBatchQuery == nil) {
          nextPageBatchQuery = [GTLBatchQuery batchQuery];
        }
        [nextPageBatchQuery addQuery:newQuery];
      }
    }
    return nextPageBatchQuery;
  }
}

// When a ticket is set to fetch more pages for feeds, this routine
// initiates the fetch for each additional feed page
- (BOOL)fetchNextPageWithQuery:(GTLQuery *)query
                      delegate:(id)delegate
           didFinishedSelector:(SEL)finishedSelector
             completionHandler:(GTLServiceCompletionHandler)completionHandler
                        ticket:(GTLServiceTicket *)ticket {
  // Sanity check the number of pages fetched already
  NSUInteger oldPagesFetchedCounter = ticket.pagesFetchedCounter;

  if (oldPagesFetchedCounter > kMaxNumberOfNextPagesFetched) {
    // Sanity check failed: way too many pages were fetched
    //
    // The client should be querying with a higher max results per page
    // to avoid this
    GTL_DEBUG_ASSERT(0, @"Fetched too many next pages for %@",
                     query.methodName);
    return NO;
  }

  ticket.pagesFetchedCounter = 1 + oldPagesFetchedCounter;

  GTLServiceTicket *newTicket;
  if (query.isBatchQuery) {
    newTicket = [self executeBatchQuery:(GTLBatchQuery *)query
                               delegate:delegate
                      didFinishSelector:finishedSelector
                      completionHandler:completionHandler
                                 ticket:ticket];
  } else {
    newTicket = [self fetchObjectWithMethodNamed:query.methodName
                                     objectClass:query.expectedObjectClass
                                      parameters:query.JSON
                                      bodyObject:query.bodyObject
                                       requestID:query.requestID
                              urlQueryParameters:query.urlQueryParameters
                                        delegate:delegate
                               didFinishSelector:finishedSelector
                               completionHandler:completionHandler
                                  executingQuery:query
                                          ticket:ticket];
  }

  // In the bizarre case that the fetch didn't begin, newTicket will be
  // nil.  So long as the new ticket is the same as the ticket we're
  // continuing, then we're happy.
  return (newTicket == ticket);
}

// Given a new single or batch result (meaning additional pages for a previous
// query result), merge it into the old result.
- (GTLObject *)mergedNewResultObject:(GTLObject *)newResult
                     oldResultObject:(GTLObject *)oldResult
                            forQuery:(GTLQuery *)query {
  if (query.isBatchQuery) {
    // Batch query result
    //
    // The new batch results are a subset of the old result's queries, since
    // not all queries in the batch necessarily have additional pages.
    //
    // New success objects replace old success objects, with the old items
    // prepended; new failure objects replace old success objects.
    // We will update the old batch results with accumulated items, using the
    // new objects, and return the old batch.
    //
    // We reuse the old batch results object because it may include some earlier
    // results which did not have additional pages.
    GTLBatchResult *newBatchResult = (GTLBatchResult *)newResult;
    GTLBatchResult *oldBatchResult = (GTLBatchResult *)oldResult;

    NSMutableDictionary *newSuccesses = newBatchResult.successes;
    NSMutableDictionary *newFailures = newBatchResult.failures;
    NSMutableDictionary *oldSuccesses = oldBatchResult.successes;
    NSMutableDictionary *oldFailures = oldBatchResult.failures;

    for (NSString *requestID in newSuccesses) {
      // Prepend the old items to the new response's items
      //
      // We can assume the objects are collections since they're present in
      // additional pages.
      GTLCollectionObject *newObj = [newSuccesses objectForKey:requestID];
      GTLCollectionObject *oldObj = [oldSuccesses objectForKey:requestID];

      NSMutableArray *items = [NSMutableArray arrayWithArray:oldObj.items];
      [items addObjectsFromArray:newObj.items];
      [newObj performSelector:@selector(setItems:) withObject:items];

      // Replace the old object with the new one
      [oldSuccesses setObject:newObj forKey:requestID];
    }

    for (NSString *requestID in newFailures) {
      // Replace old successes or failures with the new failure
      GTLErrorObject *newError = [newFailures objectForKey:requestID];
      [oldFailures setObject:newError forKey:requestID];
      [oldSuccesses removeObjectForKey:requestID];
    }
    return oldBatchResult;
  } else {
    // Single query result
    //
    // Merge the items into the new object, and return that.
    //
    // We can assume the objects are collections since they're present in
    // additional pages.
    GTLCollectionObject *newObj = (GTLCollectionObject *)newResult;
    GTLCollectionObject *oldObj = (GTLCollectionObject *)oldResult;

    NSMutableArray *items = [NSMutableArray arrayWithArray:oldObj.items];
    [items addObjectsFromArray:newObj.items];
    [newObj performSelector:@selector(setItems:) withObject:items];

    return newObj;
  }
}

#pragma mark -

// GTLQuery methods.

- (GTLServiceTicket *)executeQuery:(id<GTLQueryProtocol>)queryObj
                          delegate:(id)delegate
                 didFinishSelector:(SEL)finishedSelector {
  if ([queryObj isBatchQuery]) {
   return [self executeBatchQuery:queryObj
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:NULL
                           ticket:nil];
  }

  GTLQuery *query = [[(GTLQuery *)queryObj copy] autorelease];
  NSString *methodName = query.methodName;
  NSDictionary *params = query.JSON;
  GTLObject *bodyObject = query.bodyObject;

  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:query.expectedObjectClass
                               parameters:params
                               bodyObject:bodyObject
                                requestID:query.requestID
                       urlQueryParameters:query.urlQueryParameters
                                 delegate:delegate
                        didFinishSelector:finishedSelector
                        completionHandler:nil
                           executingQuery:query
                                   ticket:nil];
}

#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)executeQuery:(id<GTLQueryProtocol>)queryObj
                 completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  if ([queryObj isBatchQuery]) {
    return [self executeBatchQuery:queryObj
                          delegate:nil
                 didFinishSelector:NULL
                 completionHandler:handler
                            ticket:nil];
  }

  GTLQuery *query = [[(GTLQuery *)queryObj copy] autorelease];
  NSString *methodName = query.methodName;
  NSDictionary *params = query.JSON;
  GTLObject *bodyObject = query.bodyObject;

  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:query.expectedObjectClass
                               parameters:params
                               bodyObject:bodyObject
                                requestID:query.requestID
                       urlQueryParameters:query.urlQueryParameters
                                 delegate:nil
                        didFinishSelector:NULL
                        completionHandler:handler
                           executingQuery:query
                                   ticket:nil];
}
#endif

#pragma mark -

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:parameters
                               bodyObject:nil
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:delegate
                        didFinishSelector:finishedSelector
                        completionHandler:nil
                           executingQuery:nil
                                   ticket:nil];
}

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:nil
                               bodyObject:bodyObject
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:delegate
                        didFinishSelector:finishedSelector
                        completionHandler:nil
                           executingQuery:nil
                                   ticket:nil];
}

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:parameters
                               bodyObject:bodyObject
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:delegate
                        didFinishSelector:finishedSelector
                        completionHandler:nil
                           executingQuery:nil
                                   ticket:nil];
}

#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:parameters
                               bodyObject:nil
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:nil
                        didFinishSelector:NULL
                        completionHandler:handler
                           executingQuery:nil
                                   ticket:nil];
}

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:nil
                               bodyObject:bodyObject
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:nil
                        didFinishSelector:NULL
                        completionHandler:handler
                           executingQuery:nil
                                   ticket:nil];
}

- (GTLServiceTicket *)fetchObjectWithMethodNamed:(NSString *)methodName
                                      parameters:(NSDictionary *)parameters
                                 insertingObject:(GTLObject *)bodyObject
                                     objectClass:(Class)objectClass
                               completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  return [self fetchObjectWithMethodNamed:methodName
                              objectClass:objectClass
                               parameters:parameters
                               bodyObject:bodyObject
                                requestID:nil
                       urlQueryParameters:nil
                                 delegate:nil
                        didFinishSelector:NULL
                        completionHandler:handler
                           executingQuery:nil
                                   ticket:nil];
}
#endif

#pragma mark -

// These external entry points doing a REST style fetch.

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)feedURL
                                  delegate:(id)delegate
                         didFinishSelector:(SEL)finishedSelector {
  // no object class specified; use registered class
  return [self fetchObjectWithURL:feedURL
                      objectClass:nil
                       bodyObject:nil
                             ETag:nil
                       httpMethod:nil
                     mayAuthorize:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}

- (GTLServiceTicket *)fetchPublicObjectWithURL:(NSURL *)feedURL
                                     objectClass:(Class)objectClass
                                        delegate:(id)delegate
                               didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithURL:feedURL
                      objectClass:objectClass
                       bodyObject:nil
                             ETag:nil
                       httpMethod:nil
                     mayAuthorize:NO
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}

- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)feedURL
                               objectClass:(Class)objectClass
                                  delegate:(id)delegate
                         didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithURL:feedURL
                      objectClass:objectClass
                       bodyObject:nil
                             ETag:nil
                       httpMethod:nil
                     mayAuthorize:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}


- (GTLServiceTicket *)fetchObjectByInsertingObject:(GTLObject *)bodyToPost
                                              forURL:(NSURL *)destinationURL
                                            delegate:(id)delegate
                                   didFinishSelector:(SEL)finishedSelector {
  Class objClass = [bodyToPost class];
  NSString *etag = ETagIfPresent(bodyToPost);

  return [self fetchObjectWithURL:destinationURL
                      objectClass:objClass
                       bodyObject:bodyToPost
                             ETag:etag
                       httpMethod:@"POST"
                     mayAuthorize:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}

- (GTLServiceTicket *)fetchObjectByUpdatingObject:(GTLObject *)bodyToPut
                                             forURL:(NSURL *)destinationURL
                                           delegate:(id)delegate
                                  didFinishSelector:(SEL)finishedSelector {
  Class objClass = [bodyToPut class];
  NSString *etag = ETagIfPresent(bodyToPut);

  return [self fetchObjectWithURL:destinationURL
                      objectClass:objClass
                       bodyObject:bodyToPut
                             ETag:etag
                       httpMethod:@"PUT"
                     mayAuthorize:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}


- (GTLServiceTicket *)deleteResourceURL:(NSURL *)destinationURL
                                     ETag:(NSString *)etagOrNil
                                 delegate:(id)delegate
                        didFinishSelector:(SEL)finishedSelector {
  return [self fetchObjectWithURL:destinationURL
                      objectClass:nil
                       bodyObject:nil
                             ETag:etagOrNil
                       httpMethod:@"DELETE"
                     mayAuthorize:YES
                         delegate:delegate
                didFinishSelector:finishedSelector
                completionHandler:nil
                           ticket:nil];
}


#if NS_BLOCKS_AVAILABLE
- (GTLServiceTicket *)fetchObjectWithURL:(NSURL *)objectURL
                       completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  return [self fetchObjectWithURL:objectURL
                      objectClass:nil
                       bodyObject:nil
                             ETag:nil
                       httpMethod:nil
                     mayAuthorize:YES
                         delegate:nil
                didFinishSelector:NULL
                completionHandler:handler
                           ticket:nil];
}

- (GTLServiceTicket *)fetchObjectByInsertingObject:(GTLObject *)bodyToPost
                                            forURL:(NSURL *)destinationURL
                                 completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  Class objClass = [bodyToPost class];
  NSString *etag = ETagIfPresent(bodyToPost);

  return [self fetchObjectWithURL:destinationURL
                      objectClass:objClass
                       bodyObject:bodyToPost
                             ETag:etag
                       httpMethod:@"POST"
                     mayAuthorize:YES
                         delegate:nil
                didFinishSelector:NULL
                completionHandler:handler
                           ticket:nil];
}

- (GTLServiceTicket *)fetchObjectByUpdatingObject:(GTLObject *)bodyToPut
                                           forURL:(NSURL *)destinationURL
                                completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  Class objClass = [bodyToPut class];
  NSString *etag = ETagIfPresent(bodyToPut);

  return [self fetchObjectWithURL:destinationURL
                      objectClass:objClass
                       bodyObject:bodyToPut
                             ETag:etag
                       httpMethod:@"PUT"
                     mayAuthorize:YES
                         delegate:nil
                didFinishSelector:NULL
                completionHandler:handler
                           ticket:nil];
}

- (GTLServiceTicket *)deleteResourceURL:(NSURL *)destinationURL
                                   ETag:(NSString *)etagOrNil
                      completionHandler:(void (^)(GTLServiceTicket *ticket, id object, NSError *error))handler {
  return [self fetchObjectWithURL:destinationURL
                      objectClass:nil
                       bodyObject:nil
                             ETag:etagOrNil
                       httpMethod:@"DELETE"
                     mayAuthorize:YES
                         delegate:nil
                didFinishSelector:NULL
                completionHandler:handler
                           ticket:nil];
}

#endif // NS_BLOCKS_AVAILABLE

#pragma mark -

- (NSString *)userAgent {
  return userAgent_;
}

- (void)setExactUserAgent:(NSString *)userAgent {
  // internal use only
  [userAgent_ release];
  userAgent_ = [userAgent copy];
}

- (void)setUserAgent:(NSString *)userAgent {
  // remove whitespace and unfriendly characters
  NSString *str = GTMCleanedUserAgentString(userAgent);
  [self setExactUserAgent:str];
}

//
// The following methods pass through to the fetcher service object
//

- (void)setCookieStorageMethod:(NSInteger)method {
  self.fetcherService.cookieStorageMethod = method;
}

- (NSInteger)cookieStorageMethod {
  return self.fetcherService.cookieStorageMethod;
}

- (void)setShouldFetchInBackground:(BOOL)flag {
  self.fetcherService.shouldFetchInBackground = flag;
}

- (BOOL)shouldFetchInBackground {
  return self.fetcherService.shouldFetchInBackground;
}

- (void)setDelegateQueue:(NSOperationQueue *)delegateQueue {
  self.fetcherService.delegateQueue = delegateQueue;
}

- (NSOperationQueue *)delegateQueue {
  return self.fetcherService.delegateQueue;
}

- (void)setRunLoopModes:(NSArray *)array {
  self.fetcherService.runLoopModes = array;
}

- (NSArray *)runLoopModes {
  return self.fetcherService.runLoopModes;
}

#pragma mark -

// The service properties becomes the initial value for each future ticket's
// properties
- (void)setServiceProperties:(NSDictionary *)dict {
  [serviceProperties_ autorelease];
  serviceProperties_ = [dict mutableCopy];
}

- (NSDictionary *)serviceProperties {
  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[serviceProperties_ retain] autorelease];
}

- (void)setServiceProperty:(id)obj forKey:(NSString *)key {

  if (obj == nil) {
    // user passed in nil, so delete the property
    [serviceProperties_ removeObjectForKey:key];
  } else {
    // be sure the property dictionary exists
    if (serviceProperties_ == nil) {
      [self setServiceProperties:[NSDictionary dictionary]];
    }
    [serviceProperties_ setObject:obj forKey:key];
  }
}

- (id)servicePropertyForKey:(NSString *)key {
  id obj = [serviceProperties_ objectForKey:key];

  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[obj retain] autorelease];
}

- (void)setServiceUserData:(id)userData {
  [self setServiceProperty:userData forKey:kUserDataPropertyKey];
}

- (id)serviceUserData {
  return [[[self servicePropertyForKey:kUserDataPropertyKey] retain] autorelease];
}

- (void)setAuthorizer:(id <GTMFetcherAuthorizationProtocol>)authorizer {
  self.fetcherService.authorizer = authorizer;
}

- (id <GTMFetcherAuthorizationProtocol>)authorizer {
  return self.fetcherService.authorizer;
}

+ (NSUInteger)defaultServiceUploadChunkSize {
  // subclasses may override
  return kStandardUploadChunkSize;
}

- (NSUInteger)serviceUploadChunkSize {
  return uploadChunkSize_;
}

- (void)setServiceUploadChunkSize:(NSUInteger)val {

  if (val == kGTLStandardUploadChunkSize) {
    // determine an appropriate upload chunk size for the system

    if (![GTMHTTPFetcher doesSupportSentDataCallback]) {
      // for 10.4 and iPhone 2, we need a small upload chunk size so there
      // are frequent intrachunk callbacks for progress monitoring
      val = 75000;
    } else {
#if GTL_IPHONE
      val = 1000000;
#else
      if (NSFoundationVersionNumber >= 751.00) {
        // Mac OS X 10.6
        //
        // we'll pick a huge upload chunk size, which minimizes http overhead
        // and server effort, and we'll hope that NSURLConnection can finally
        // handle big uploads reliably
        val = 25000000;
      } else {
        // Mac OS X 10.5
        //
        // NSURLConnection is more reliable on POSTs in 10.5 than it was in
        // 10.4, but it still fails mysteriously on big uploads on some
        // systems, so we'll limit the chunks to a megabyte
        val = 1000000;
      }
#endif
    }
  }
  uploadChunkSize_ = val;
}

@end

@implementation GTLServiceTicket

@synthesize shouldFetchNextPages = shouldFetchNextPages_,
  surrogates = surrogates_,
  uploadProgressSelector = uploadProgressSelector_,
  retryEnabled = isRetryEnabled_,
  hasCalledCallback = hasCalledCallback_,
  retrySelector = retrySelector_,
  maxRetryInterval = maxRetryInterval_,
  objectFetcher = objectFetcher_,
  postedObject = postedObject_,
  fetchedObject = fetchedObject_,
  executingQuery = executingQuery_,
  originalQuery = originalQuery_,
  fetchError = fetchError_,
  pagesFetchedCounter = pagesFetchedCounter_,
  APIKey = apiKey_,
  parseOperation = parseOperation_,
  isREST = isREST_;

#if NS_BLOCKS_AVAILABLE
@synthesize retryBlock = retryBlock_;
#endif

+ (id)ticketForService:(GTLService *)service {
  return [[[self alloc] initWithService:service] autorelease];
}

- (id)initWithService:(GTLService *)service {
  self = [super init];
  if (self) {
    service_ = [service retain];

    ticketProperties_ = [service.serviceProperties mutableCopy];
    surrogates_ = [service.surrogates retain];
    uploadProgressSelector_ = service.uploadProgressSelector;
    isRetryEnabled_ = service.retryEnabled;
    retrySelector_ = service.retrySelector;
    maxRetryInterval_ = service.maxRetryInterval;
    shouldFetchNextPages_ = service.shouldFetchNextPages;
    apiKey_ = [service.APIKey copy];

#if NS_BLOCKS_AVAILABLE
    uploadProgressBlock_ = [service.uploadProgressBlock copy];
    retryBlock_ = [service.retryBlock copy];
#endif
  }
  return self;
}

- (void)dealloc {
  [service_ release];
  [ticketProperties_ release];
  [surrogates_ release];
  [objectFetcher_ release];
#if NS_BLOCKS_AVAILABLE
  [uploadProgressBlock_ release];
  [retryBlock_ release];
#endif
  [postedObject_ release];
  [fetchedObject_ release];
  [executingQuery_ release];
  [originalQuery_ release];
  [fetchError_ release];
  [apiKey_ release];
  [parseOperation_ release];

  [super dealloc];
}

- (NSString *)description {
  NSString *devKeyInfo = @"";
  if (apiKey_ != nil) {
    devKeyInfo = [NSString stringWithFormat:@" devKey:%@", apiKey_];
  }

  NSString *authorizerInfo = @"";
  id <GTMFetcherAuthorizationProtocol> authorizer = self.objectFetcher.authorizer;
  if (authorizer != nil) {
    authorizerInfo = [NSString stringWithFormat:@" authorizer:%@", authorizer];
  }

  return [NSString stringWithFormat:@"%@ %p: {service:%@%@%@ fetcher:%@ }",
    [self class], self, service_, devKeyInfo, authorizerInfo, objectFetcher_];
}

- (void)pauseUpload {
  BOOL canPause = [objectFetcher_ respondsToSelector:@selector(pauseFetching)];
  GTL_DEBUG_ASSERT(canPause, @"unpauseable ticket");

  if (canPause) {
    [(GTMHTTPUploadFetcher *)objectFetcher_ pauseFetching];
  }
}

- (void)resumeUpload {
  BOOL canResume = [objectFetcher_ respondsToSelector:@selector(resumeFetching)];
  GTL_DEBUG_ASSERT(canResume, @"unresumable ticket");

  if (canResume) {
    [(GTMHTTPUploadFetcher *)objectFetcher_ resumeFetching];
  }
}

- (BOOL)isUploadPaused {
  BOOL isPausable = [objectFetcher_ respondsToSelector:@selector(isPaused)];
  GTL_DEBUG_ASSERT(isPausable, @"unpauseable ticket");

  if (isPausable) {
    return [(GTMHTTPUploadFetcher *)objectFetcher_ isPaused];
  }
  return NO;
}

- (void)cancelTicket {
  NSOperation *parseOperation = self.parseOperation;
  [parseOperation cancel];
  self.parseOperation = nil;

  [objectFetcher_ stopFetching];
  objectFetcher_.properties = nil;

  self.objectFetcher = nil;
  self.properties = nil;
  self.uploadProgressSelector = nil;

#if NS_BLOCKS_AVAILABLE
  self.uploadProgressBlock = nil;
  self.retryBlock = nil;
#endif
  [self.executingQuery executionDidStop];
  self.executingQuery = self.originalQuery;

  [service_ autorelease];
  service_ = nil;
}

- (id)service {
  return service_;
}

- (void)setUserData:(id)userData {
  [self setProperty:userData forKey:kUserDataPropertyKey];
}

- (id)userData {
  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[[self propertyForKey:kUserDataPropertyKey] retain] autorelease];
}

- (void)setProperties:(NSDictionary *)dict {
  [ticketProperties_ autorelease];
  ticketProperties_ = [dict mutableCopy];
}

- (NSDictionary *)properties {
  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[ticketProperties_ retain] autorelease];
}

- (void)setProperty:(id)obj forKey:(NSString *)key {
  if (obj == nil) {
    // user passed in nil, so delete the property
    [ticketProperties_ removeObjectForKey:key];
  } else {
    // be sure the property dictionary exists
    if (ticketProperties_ == nil) {
      // call setProperties so observers are notified
      [self setProperties:[NSDictionary dictionary]];
    }
    [ticketProperties_ setObject:obj forKey:key];
  }
}

- (id)propertyForKey:(NSString *)key {
  id obj = [ticketProperties_ objectForKey:key];

  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[obj retain] autorelease];
}

- (NSDictionary *)surrogates {
  return surrogates_;
}

- (void)setSurrogates:(NSDictionary *)dict {
  [surrogates_ autorelease];
  surrogates_ = [dict retain];
}

- (SEL)uploadProgressSelector {
  return uploadProgressSelector_;
}

- (void)setUploadProgressSelector:(SEL)progressSelector {
  uploadProgressSelector_ = progressSelector;

  // if the user is turning on the progress selector in the ticket after the
  // ticket's fetcher has been created, we need to give the fetcher our sentData
  // callback.
  //
  // The progress monitor must be set in the service prior to creation of the
  // ticket on 10.4 and iPhone 2.0, since on those systems the upload data must
  // be wrapped with a ProgressMonitorInputStream prior to the creation of the
  // fetcher.
  if (progressSelector != NULL) {
    SEL sentDataSel = @selector(objectFetcher:didSendBytes:totalBytesSent:totalBytesExpectedToSend:);
    [[self objectFetcher] setSentDataSelector:sentDataSel];
  }
}

#if NS_BLOCKS_AVAILABLE
- (void)setUploadProgressBlock:(GTLServiceUploadProgressBlock)block {
  [uploadProgressBlock_ autorelease];
  uploadProgressBlock_ = [block copy];

  if (uploadProgressBlock_) {
    // As above, we need the fetcher to call us back when bytes are sent.
    SEL sentDataSel = @selector(objectFetcher:didSendBytes:totalBytesSent:totalBytesExpectedToSend:);
    [[self objectFetcher] setSentDataSelector:sentDataSel];
  }
}

- (GTLServiceUploadProgressBlock)uploadProgressBlock {
  return uploadProgressBlock_;
}
#endif

- (NSInteger)statusCode {
  return [objectFetcher_ statusCode];
}

- (GTLQuery *)queryForRequestID:(NSString *)requestID {
  id<GTLQueryProtocol> queryObj = self.executingQuery;
  if ([queryObj isBatchQuery]) {
    GTLBatchQuery *batch = (GTLBatchQuery *)queryObj;
    GTLQuery *result = [batch queryForRequestID:requestID];
    return result;
  } else {
    GTL_DEBUG_ASSERT(0, @"just use ticket.executingQuery");
    return nil;
  }
}

@end

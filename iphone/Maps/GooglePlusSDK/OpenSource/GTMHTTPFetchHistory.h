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
//  GTMHTTPFetchHistory.h
//

//
// Users of the GTMHTTPFetcher class may optionally create and set a fetch
// history object.  The fetch history provides "memory" between subsequent
// fetches, including:
//
// - For fetch responses with Etag headers, the fetch history
//   remembers the response headers. Future fetcher requests to the same URL
//   will be given an "If-None-Match" header, telling the server to return
//   a 304 Not Modified status if the response is unchanged, reducing the
//   server load and network traffic.
//
// - Optionally, the fetch history can cache the ETagged data that was returned
//   in the responses that contained Etag headers. If a later fetch
//   results in a 304 status, the fetcher will return the cached ETagged data
//   to the client along with a 200 status, hiding the 304.
//
// - The fetch history can track cookies.
//

#pragma once

#import <Foundation/Foundation.h>

#import "GTMHTTPFetcher.h"

#undef _EXTERN
#undef _INITIALIZE_AS
#ifdef GTMHTTPFETCHHISTORY_DEFINE_GLOBALS
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


// default data cache size for when we're caching responses to handle "not
// modified" errors for the client
#if GTM_IPHONE
// iPhone: up to 1MB memory
_EXTERN const NSUInteger kGTMDefaultETaggedDataCacheMemoryCapacity _INITIALIZE_AS(1*1024*1024);
#else
// Mac OS X: up to 15MB memory
_EXTERN const NSUInteger kGTMDefaultETaggedDataCacheMemoryCapacity _INITIALIZE_AS(15*1024*1024);
#endif

// forward declarations
@class GTMURLCache;
@class GTMCookieStorage;

@interface GTMHTTPFetchHistory : NSObject <GTMHTTPFetchHistoryProtocol> {
 @private
  GTMURLCache *etaggedDataCache_;
  BOOL shouldRememberETags_;
  BOOL shouldCacheETaggedData_;        // if NO, then only headers are cached
  GTMCookieStorage *cookieStorage_;
}

// With caching enabled, previously-cached data will be returned instead of
// 304 Not Modified responses when repeating a fetch of an URL that previously
// included an ETag header in its response
@property (assign) BOOL shouldRememberETags;     // default: NO
@property (assign) BOOL shouldCacheETaggedData;  // default: NO

// the default ETag data cache capacity is kGTMDefaultETaggedDataCacheMemoryCapacity
@property (assign) NSUInteger memoryCapacity;

@property (retain) GTMCookieStorage *cookieStorage;

- (id)initWithMemoryCapacity:(NSUInteger)totalBytes
      shouldCacheETaggedData:(BOOL)shouldCacheETaggedData;

- (void)updateRequest:(NSMutableURLRequest *)request isHTTPGet:(BOOL)isHTTPGet;

- (void)clearETaggedDataCache;
- (void)clearHistory;

- (void)removeAllCookies;

@end


// GTMURLCache and GTMCachedURLResponse have interfaces similar to their
// NSURLCache counterparts, in hopes that someday the NSURLCache versions
// can be used. But in 10.5.8, those are not reliable enough except when
// used with +setSharedURLCache. Our goal here is just to cache
// responses for handling If-None-Match requests that return
// "Not Modified" responses, not for replacing the general URL
// caches.

@interface GTMCachedURLResponse : NSObject {
 @private
  NSURLResponse *response_;
  NSData *data_;
  NSDate *useDate_;         // date this response was last saved or used
  NSDate *reservationDate_; // date this response's ETag was used
}

@property (readonly) NSURLResponse* response;
@property (readonly) NSData* data;

// date the response was saved or last accessed
@property (retain) NSDate *useDate;

// date the response's ETag header was last used for a fetch request
@property (retain) NSDate *reservationDate;

- (id)initWithResponse:(NSURLResponse *)response data:(NSData *)data;
@end

@interface GTMURLCache : NSObject {
  NSMutableDictionary *responses_; // maps request URL to GTMCachedURLResponse
  NSUInteger memoryCapacity_;      // capacity of NSDatas in the responses
  NSUInteger totalDataSize_;       // sum of sizes of NSDatas of all responses
  NSTimeInterval reservationInterval_; // reservation expiration interval
}

@property (assign) NSUInteger memoryCapacity;

- (id)initWithMemoryCapacity:(NSUInteger)totalBytes;

- (GTMCachedURLResponse *)cachedResponseForRequest:(NSURLRequest *)request;
- (void)storeCachedResponse:(GTMCachedURLResponse *)cachedResponse forRequest:(NSURLRequest *)request;
- (void)removeCachedResponseForRequest:(NSURLRequest *)request;
- (void)removeAllCachedResponses;

// for unit testing
- (void)setReservationInterval:(NSTimeInterval)secs;
- (NSDictionary *)responses;
- (NSUInteger)totalDataSize;
@end

@interface GTMCookieStorage : NSObject <GTMCookieStorageProtocol> {
 @private
  // The cookie storage object manages an array holding cookies, but the array
  // is allocated externally (it may be in a fetcher object or the static
  // fetcher cookie array.)  See the fetcher's setCookieStorageMethod:
  // for allocation of this object and assignment of its cookies array.
  NSMutableArray *cookies_;
}

// add all NSHTTPCookies in the supplied array to the storage array,
// replacing cookies in the storage array as appropriate
// Side effect: removes expired cookies from the storage array
- (void)setCookies:(NSArray *)newCookies;

// retrieve all cookies appropriate for the given URL, considering
// domain, path, cookie name, expiration, security setting.
// Side effect: removes expired cookies from the storage array
- (NSArray *)cookiesForURL:(NSURL *)theURL;

// return a cookie with the same name, domain, and path as the
// given cookie, or else return nil if none found
//
// Both the cookie being tested and all stored cookies should
// be valid (non-nil name, domains, paths)
- (NSHTTPCookie *)cookieMatchingCookie:(NSHTTPCookie *)cookie;

// remove any expired cookies, excluding cookies with nil expirations
- (void)removeExpiredCookies;

- (void)removeAllCookies;

@end

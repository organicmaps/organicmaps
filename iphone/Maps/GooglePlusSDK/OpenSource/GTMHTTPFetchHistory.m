/* Copyright (c) 2010 Google Inc.
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
//  GTMHTTPFetchHistory.m
//

#define GTMHTTPFETCHHISTORY_DEFINE_GLOBALS 1

#import "GTMHTTPFetchHistory.h"

const NSTimeInterval kCachedURLReservationInterval = 60.0; // 1 minute
static NSString* const kGTMIfNoneMatchHeader = @"If-None-Match";
static NSString* const kGTMETagHeader = @"Etag";

@implementation GTMCookieStorage

- (id)init {
  self = [super init];
  if (self != nil) {
    cookies_ = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)dealloc {
  [cookies_ release];
  [super dealloc];
}

// Add all cookies in the new cookie array to the storage,
// replacing stored cookies as appropriate.
//
// Side effect: removes expired cookies from the storage array.
- (void)setCookies:(NSArray *)newCookies {

  @synchronized(cookies_) {
    [self removeExpiredCookies];

    for (NSHTTPCookie *newCookie in newCookies) {
      if ([[newCookie name] length] > 0
          && [[newCookie domain] length] > 0
          && [[newCookie path] length] > 0) {

        // remove the cookie if it's currently in the array
        NSHTTPCookie *oldCookie = [self cookieMatchingCookie:newCookie];
        if (oldCookie) {
          [cookies_ removeObjectIdenticalTo:oldCookie];
        }

        // make sure the cookie hasn't already expired
        NSDate *expiresDate = [newCookie expiresDate];
        if ((!expiresDate) || [expiresDate timeIntervalSinceNow] > 0) {
          [cookies_ addObject:newCookie];
        }

      } else {
        NSAssert1(NO, @"Cookie incomplete: %@", newCookie);
      }
    }
  }
}

- (void)deleteCookie:(NSHTTPCookie *)cookie {
  @synchronized(cookies_) {
    NSHTTPCookie *foundCookie = [self cookieMatchingCookie:cookie];
    if (foundCookie) {
      [cookies_ removeObjectIdenticalTo:foundCookie];
    }
  }
}

// Retrieve all cookies appropriate for the given URL, considering
// domain, path, cookie name, expiration, security setting.
// Side effect: removed expired cookies from the storage array.
- (NSArray *)cookiesForURL:(NSURL *)theURL {

  NSMutableArray *foundCookies = nil;

  @synchronized(cookies_) {
    [self removeExpiredCookies];

    // We'll prepend "." to the desired domain, since we want the
    // actual domain "nytimes.com" to still match the cookie domain
    // ".nytimes.com" when we check it below with hasSuffix.
    NSString *host = [[theURL host] lowercaseString];
    NSString *path = [theURL path];
    NSString *scheme = [theURL scheme];

    NSString *domain = nil;
    BOOL isLocalhostRetrieval = NO;

    if ([host isEqual:@"localhost"]) {
      isLocalhostRetrieval = YES;
    } else {
      if (host) {
        domain = [@"." stringByAppendingString:host];
      }
    }

    NSUInteger numberOfCookies = [cookies_ count];
    for (NSUInteger idx = 0; idx < numberOfCookies; idx++) {

      NSHTTPCookie *storedCookie = [cookies_ objectAtIndex:idx];

      NSString *cookieDomain = [[storedCookie domain] lowercaseString];
      NSString *cookiePath = [storedCookie path];
      BOOL cookieIsSecure = [storedCookie isSecure];

      BOOL isDomainOK;

      if (isLocalhostRetrieval) {
        // prior to 10.5.6, the domain stored into NSHTTPCookies for localhost
        // is "localhost.local"
        isDomainOK = [cookieDomain isEqual:@"localhost"]
          || [cookieDomain isEqual:@"localhost.local"];
      } else {
        isDomainOK = [domain hasSuffix:cookieDomain];
      }

      BOOL isPathOK = [cookiePath isEqual:@"/"] || [path hasPrefix:cookiePath];
      BOOL isSecureOK = (!cookieIsSecure) || [scheme isEqual:@"https"];

      if (isDomainOK && isPathOK && isSecureOK) {
        if (foundCookies == nil) {
          foundCookies = [NSMutableArray arrayWithCapacity:1];
        }
        [foundCookies addObject:storedCookie];
      }
    }
  }
  return foundCookies;
}

// Return a cookie from the array with the same name, domain, and path as the
// given cookie, or else return nil if none found.
//
// Both the cookie being tested and all cookies in the storage array should
// be valid (non-nil name, domains, paths).
//
// Note: this should only be called from inside a @synchronized(cookies_) block
- (NSHTTPCookie *)cookieMatchingCookie:(NSHTTPCookie *)cookie {

  NSUInteger numberOfCookies = [cookies_ count];
  NSString *name = [cookie name];
  NSString *domain = [cookie domain];
  NSString *path = [cookie path];

  NSAssert3(name && domain && path, @"Invalid cookie (name:%@ domain:%@ path:%@)",
            name, domain, path);

  for (NSUInteger idx = 0; idx < numberOfCookies; idx++) {

    NSHTTPCookie *storedCookie = [cookies_ objectAtIndex:idx];

    if ([[storedCookie name] isEqual:name]
        && [[storedCookie domain] isEqual:domain]
        && [[storedCookie path] isEqual:path]) {

      return storedCookie;
    }
  }
  return nil;
}


// Internal routine to remove any expired cookies from the array, excluding
// cookies with nil expirations.
//
// Note: this should only be called from inside a @synchronized(cookies_) block
- (void)removeExpiredCookies {

  // count backwards since we're deleting items from the array
  for (NSInteger idx = (NSInteger)[cookies_ count] - 1; idx >= 0; idx--) {

    NSHTTPCookie *storedCookie = [cookies_ objectAtIndex:(NSUInteger)idx];

    NSDate *expiresDate = [storedCookie expiresDate];
    if (expiresDate && [expiresDate timeIntervalSinceNow] < 0) {
      [cookies_ removeObjectAtIndex:(NSUInteger)idx];
    }
  }
}

- (void)removeAllCookies {
  @synchronized(cookies_) {
    [cookies_ removeAllObjects];
  }
}
@end

//
// GTMCachedURLResponse
//

@implementation GTMCachedURLResponse

@synthesize response = response_;
@synthesize data = data_;
@synthesize reservationDate = reservationDate_;
@synthesize useDate = useDate_;

- (id)initWithResponse:(NSURLResponse *)response data:(NSData *)data {
  self = [super init];
  if (self != nil) {
    response_ = [response retain];
    data_ = [data retain];
    useDate_ = [[NSDate alloc] init];
  }
  return self;
}

- (void)dealloc {
  [response_ release];
  [data_ release];
  [useDate_ release];
  [reservationDate_ release];
  [super dealloc];
}

- (NSString *)description {
  NSString *reservationStr = reservationDate_ ?
    [NSString stringWithFormat:@" resDate:%@", reservationDate_] : @"";

  return [NSString stringWithFormat:@"%@ %p: {bytes:%@ useDate:%@%@}",
          [self class], self,
          data_ ? [NSNumber numberWithInt:(int)[data_ length]] : nil,
          useDate_,
          reservationStr];
}

- (NSComparisonResult)compareUseDate:(GTMCachedURLResponse *)other {
  return [useDate_ compare:[other useDate]];
}

@end

//
// GTMURLCache
//

@implementation GTMURLCache

@dynamic memoryCapacity;

- (id)init {
  return [self initWithMemoryCapacity:kGTMDefaultETaggedDataCacheMemoryCapacity];
}

- (id)initWithMemoryCapacity:(NSUInteger)totalBytes {
  self = [super init];
  if (self != nil) {
    memoryCapacity_ = totalBytes;

    responses_ = [[NSMutableDictionary alloc] initWithCapacity:5];

    reservationInterval_ = kCachedURLReservationInterval;
  }
  return self;
}

- (void)dealloc {
  [responses_ release];
  [super dealloc];
}

- (NSString *)description {
  return [NSString stringWithFormat:@"%@ %p: {responses:%@}",
          [self class], self, [responses_ allValues]];
}

// Setters/getters

- (void)pruneCacheResponses {
  // Internal routine to remove the least-recently-used responses when the
  // cache has grown too large
  if (memoryCapacity_ >= totalDataSize_) return;

  // Sort keys by date
  SEL sel = @selector(compareUseDate:);
  NSArray *sortedKeys = [responses_ keysSortedByValueUsingSelector:sel];

  // The least-recently-used keys are at the beginning of the sorted array;
  // remove those (except ones still reserved) until the total data size is
  // reduced sufficiently
  for (NSURL *key in sortedKeys) {
    GTMCachedURLResponse *response = [responses_ objectForKey:key];

    NSDate *resDate = [response reservationDate];
    BOOL isResponseReserved = (resDate != nil)
      && ([resDate timeIntervalSinceNow] > -reservationInterval_);

    if (!isResponseReserved) {
      // We can remove this response from the cache
      NSUInteger storedSize = [[response data] length];
      totalDataSize_ -= storedSize;
      [responses_ removeObjectForKey:key];
    }

    // If we've removed enough response data, then we're done
    if (memoryCapacity_ >= totalDataSize_) break;
  }
}

- (void)storeCachedResponse:(GTMCachedURLResponse *)cachedResponse
                 forRequest:(NSURLRequest *)request {
  @synchronized(self) {
    // Remove any previous entry for this request
    [self removeCachedResponseForRequest:request];

    // cache this one only if it's not bigger than our cache
    NSUInteger storedSize = [[cachedResponse data] length];
    if (storedSize < memoryCapacity_) {

      NSURL *key = [request URL];
      [responses_ setObject:cachedResponse forKey:key];
      totalDataSize_ += storedSize;

      [self pruneCacheResponses];
    }
  }
}

- (GTMCachedURLResponse *)cachedResponseForRequest:(NSURLRequest *)request {
  GTMCachedURLResponse *response;

  @synchronized(self) {
    NSURL *key = [request URL];
    response = [[[responses_ objectForKey:key] retain] autorelease];

    // Touch the date to indicate this was recently retrieved
    [response setUseDate:[NSDate date]];
  }
  return response;
}

- (void)removeCachedResponseForRequest:(NSURLRequest *)request {
  @synchronized(self) {
    NSURL *key = [request URL];
    totalDataSize_ -= [[[responses_ objectForKey:key] data] length];
    [responses_ removeObjectForKey:key];
  }
}

- (void)removeAllCachedResponses {
  @synchronized(self) {
    [responses_ removeAllObjects];
    totalDataSize_ = 0;
  }
}

- (NSUInteger)memoryCapacity {
  return memoryCapacity_;
}

- (void)setMemoryCapacity:(NSUInteger)totalBytes {
  @synchronized(self) {
    BOOL didShrink = (totalBytes < memoryCapacity_);
    memoryCapacity_ = totalBytes;

    if (didShrink) {
      [self pruneCacheResponses];
    }
  }
}

// Methods for unit testing.
- (void)setReservationInterval:(NSTimeInterval)secs {
  reservationInterval_ = secs;
}

- (NSDictionary *)responses {
  return responses_;
}

- (NSUInteger)totalDataSize {
  return totalDataSize_;
}

@end

//
// GTMHTTPFetchHistory
//

@interface GTMHTTPFetchHistory ()
- (NSString *)cachedETagForRequest:(NSURLRequest *)request;
- (void)removeCachedDataForRequest:(NSURLRequest *)request;
@end

@implementation GTMHTTPFetchHistory

@synthesize cookieStorage = cookieStorage_;

@dynamic shouldRememberETags;
@dynamic shouldCacheETaggedData;
@dynamic memoryCapacity;

- (id)init {
 return [self initWithMemoryCapacity:kGTMDefaultETaggedDataCacheMemoryCapacity
              shouldCacheETaggedData:NO];
}

- (id)initWithMemoryCapacity:(NSUInteger)totalBytes
      shouldCacheETaggedData:(BOOL)shouldCacheETaggedData {
  self = [super init];
  if (self != nil) {
    etaggedDataCache_ = [[GTMURLCache alloc] initWithMemoryCapacity:totalBytes];
    shouldRememberETags_ = shouldCacheETaggedData;
    shouldCacheETaggedData_ = shouldCacheETaggedData;
    cookieStorage_ = [[GTMCookieStorage alloc] init];
  }
  return self;
}

- (void)dealloc {
  [etaggedDataCache_ release];
  [cookieStorage_ release];
  [super dealloc];
}

- (void)updateRequest:(NSMutableURLRequest *)request isHTTPGet:(BOOL)isHTTPGet {
  @synchronized(self) {
    if ([self shouldRememberETags]) {
      // If this URL is in the history, and no ETag has been set, then
      // set the ETag header field

      // If we have a history, we're tracking across fetches, so we don't
      // want to pull results from any other cache
      [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];

      if (isHTTPGet) {
        // We'll only add an ETag if there's no ETag specified in the user's
        // request
        NSString *specifiedETag = [request valueForHTTPHeaderField:kGTMIfNoneMatchHeader];
        if (specifiedETag == nil) {
          // No ETag: extract the previous ETag for this request from the
          // fetch history, and add it to the request
          NSString *cachedETag = [self cachedETagForRequest:request];

          if (cachedETag != nil) {
            [request addValue:cachedETag forHTTPHeaderField:kGTMIfNoneMatchHeader];
          }
        } else {
          // Has an ETag: remove any stored response in the fetch history
          // for this request, as the If-None-Match header could lead to
          // a 304 Not Modified, and we want that error delivered to the
          // user since they explicitly specified the ETag
          [self removeCachedDataForRequest:request];
        }
      }
    }
  }
}

- (void)updateFetchHistoryWithRequest:(NSURLRequest *)request
                             response:(NSURLResponse *)response
                       downloadedData:(NSData *)downloadedData {
  @synchronized(self) {
    if (![self shouldRememberETags]) return;

    if (![response respondsToSelector:@selector(allHeaderFields)]) return;

    NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];

    if (statusCode != kGTMHTTPFetcherStatusNotModified) {
      // Save this ETag string for successful results (<300)
      // If there's no last modified string, clear the dictionary
      // entry for this URL. Also cache or delete the data, if appropriate
      // (when etaggedDataCache is non-nil.)
      NSDictionary *headers = [(NSHTTPURLResponse *)response allHeaderFields];
      NSString* etag = [headers objectForKey:kGTMETagHeader];

      if (etag != nil && statusCode < 300) {

        // we want to cache responses for the headers, even if the client
        // doesn't want the response body data caches
        NSData *dataToStore = shouldCacheETaggedData_ ? downloadedData : nil;

        GTMCachedURLResponse *cachedResponse;
        cachedResponse = [[[GTMCachedURLResponse alloc] initWithResponse:response
                                                                      data:dataToStore] autorelease];
        [etaggedDataCache_ storeCachedResponse:cachedResponse
                                  forRequest:request];
      } else {
        [etaggedDataCache_ removeCachedResponseForRequest:request];
      }
    }
  }
}

- (NSString *)cachedETagForRequest:(NSURLRequest *)request {
  // Internal routine.
  GTMCachedURLResponse *cachedResponse;
  cachedResponse = [etaggedDataCache_ cachedResponseForRequest:request];

  NSURLResponse *response = [cachedResponse response];
  NSDictionary *headers = [(NSHTTPURLResponse *)response allHeaderFields];
  NSString *cachedETag = [headers objectForKey:kGTMETagHeader];
  if (cachedETag) {
    // Since the request having an ETag implies this request is about
    // to be fetched again, reserve the cached response to ensure that
    // that it will be around at least until the fetch completes.
    //
    // When the fetch completes, either the cached response will be replaced
    // with a new response, or the cachedDataForRequest: method below will
    // clear the reservation.
    [cachedResponse setReservationDate:[NSDate date]];
  }
  return cachedETag;
}

- (NSData *)cachedDataForRequest:(NSURLRequest *)request {
  @synchronized(self) {
    GTMCachedURLResponse *cachedResponse;
    cachedResponse = [etaggedDataCache_ cachedResponseForRequest:request];

    NSData *cachedData = [cachedResponse data];

    // Since the data for this cached request is being obtained from the cache,
    // we can clear the reservation as the fetch has completed.
    [cachedResponse setReservationDate:nil];

    return cachedData;
  }
}

- (void)removeCachedDataForRequest:(NSURLRequest *)request {
  @synchronized(self) {
    [etaggedDataCache_ removeCachedResponseForRequest:request];
  }
}

- (void)clearETaggedDataCache {
  @synchronized(self) {
    [etaggedDataCache_ removeAllCachedResponses];
  }
}

- (void)clearHistory {
  @synchronized(self) {
    [self clearETaggedDataCache];
    [cookieStorage_ removeAllCookies];
  }
}

- (void)removeAllCookies {
  @synchronized(self) {
    [cookieStorage_ removeAllCookies];
  }
}

- (BOOL)shouldRememberETags {
  return shouldRememberETags_;
}

- (void)setShouldRememberETags:(BOOL)flag {
  BOOL wasRemembering = shouldRememberETags_;
  shouldRememberETags_ = flag;

  if (wasRemembering && !flag) {
    // Free up the cache memory
    [self clearETaggedDataCache];
  }
}

- (BOOL)shouldCacheETaggedData {
  return shouldCacheETaggedData_;
}

- (void)setShouldCacheETaggedData:(BOOL)flag {
  BOOL wasCaching = shouldCacheETaggedData_;
  shouldCacheETaggedData_ = flag;

  if (flag) {
    self.shouldRememberETags = YES;
  }

  if (wasCaching && !flag) {
    // users expect turning off caching to free up the cache memory
    [self clearETaggedDataCache];
  }
}

- (NSUInteger)memoryCapacity {
  return [etaggedDataCache_ memoryCapacity];
}

- (void)setMemoryCapacity:(NSUInteger)totalBytes {
  [etaggedDataCache_ setMemoryCapacity:totalBytes];
}

@end

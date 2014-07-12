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
//  GTMHTTPFetcherService.m
//

#import "GTMHTTPFetcherService.h"

@interface GTMHTTPFetcher (ServiceMethods)
- (BOOL)beginFetchMayDelay:(BOOL)mayDelay
              mayAuthorize:(BOOL)mayAuthorize;
@end

@interface GTMHTTPFetcherService ()
@property (retain, readwrite) NSDictionary *delayedHosts;
@property (retain, readwrite) NSDictionary *runningHosts;

- (void)detachAuthorizer;
@end

@implementation GTMHTTPFetcherService

@synthesize maxRunningFetchersPerHost = maxRunningFetchersPerHost_,
            userAgent = userAgent_,
            timeout = timeout_,
            delegateQueue = delegateQueue_,
            runLoopModes = runLoopModes_,
            credential = credential_,
            proxyCredential = proxyCredential_,
            cookieStorageMethod = cookieStorageMethod_,
            shouldFetchInBackground = shouldFetchInBackground_,
            fetchHistory = fetchHistory_;

- (id)init {
  self = [super init];
  if (self) {
    fetchHistory_ = [[GTMHTTPFetchHistory alloc] init];
    delayedHosts_ = [[NSMutableDictionary alloc] init];
    runningHosts_ = [[NSMutableDictionary alloc] init];
    cookieStorageMethod_ = kGTMHTTPFetcherCookieStorageMethodFetchHistory;

    maxRunningFetchersPerHost_ = 10;
}
  return self;
}

- (void)dealloc {
  [self detachAuthorizer];

  [delayedHosts_ release];
  [runningHosts_ release];
  [fetchHistory_ release];
  [userAgent_ release];
  [delegateQueue_ release];
  [runLoopModes_ release];
  [credential_ release];
  [proxyCredential_ release];
  [authorizer_ release];

  [super dealloc];
}

#pragma mark Generate a new fetcher

- (id)fetcherWithRequest:(NSURLRequest *)request
            fetcherClass:(Class)fetcherClass {
  GTMHTTPFetcher *fetcher = [fetcherClass fetcherWithRequest:request];

  fetcher.fetchHistory = self.fetchHistory;
  fetcher.delegateQueue = self.delegateQueue;
  fetcher.runLoopModes = self.runLoopModes;
  fetcher.cookieStorageMethod = self.cookieStorageMethod;
  fetcher.credential = self.credential;
  fetcher.proxyCredential = self.proxyCredential;
  fetcher.shouldFetchInBackground = self.shouldFetchInBackground;
  fetcher.authorizer = self.authorizer;
  fetcher.service = self;

  NSString *userAgent = self.userAgent;
  if ([userAgent length] > 0
      && [request valueForHTTPHeaderField:@"User-Agent"] == nil) {
    [fetcher.mutableRequest setValue:userAgent
                  forHTTPHeaderField:@"User-Agent"];
  }

  NSTimeInterval timeout = self.timeout;
  if (timeout > 0.0) {
    [fetcher.mutableRequest setTimeoutInterval:timeout];
  }

  return fetcher;
}

- (GTMHTTPFetcher *)fetcherWithRequest:(NSURLRequest *)request {
  return [self fetcherWithRequest:request
                     fetcherClass:[GTMHTTPFetcher class]];
}

- (GTMHTTPFetcher *)fetcherWithURL:(NSURL *)requestURL {
  return [self fetcherWithRequest:[NSURLRequest requestWithURL:requestURL]];
}

- (GTMHTTPFetcher *)fetcherWithURLString:(NSString *)requestURLString {
  return [self fetcherWithURL:[NSURL URLWithString:requestURLString]];
}

#pragma mark Queue Management

- (void)addRunningFetcher:(GTMHTTPFetcher *)fetcher
                  forHost:(NSString *)host {
  // Add to the array of running fetchers for this host, creating the array
  // if needed
  NSMutableArray *runningForHost = [runningHosts_ objectForKey:host];
  if (runningForHost == nil) {
    runningForHost = [NSMutableArray arrayWithObject:fetcher];
    [runningHosts_ setObject:runningForHost forKey:host];
  } else {
    [runningForHost addObject:fetcher];
  }
}

- (void)addDelayedFetcher:(GTMHTTPFetcher *)fetcher
                  forHost:(NSString *)host {
  // Add to the array of delayed fetchers for this host, creating the array
  // if needed
  NSMutableArray *delayedForHost = [delayedHosts_ objectForKey:host];
  if (delayedForHost == nil) {
    delayedForHost = [NSMutableArray arrayWithObject:fetcher];
    [delayedHosts_ setObject:delayedForHost forKey:host];
  } else {
    [delayedForHost addObject:fetcher];
  }
}

- (BOOL)isDelayingFetcher:(GTMHTTPFetcher *)fetcher {
  @synchronized(self) {
    NSString *host = [[[fetcher mutableRequest] URL] host];
    NSArray *delayedForHost = [delayedHosts_ objectForKey:host];
    NSUInteger idx = [delayedForHost indexOfObjectIdenticalTo:fetcher];
    BOOL isDelayed = (delayedForHost != nil) && (idx != NSNotFound);
    return isDelayed;
  }
}

- (BOOL)fetcherShouldBeginFetching:(GTMHTTPFetcher *)fetcher {
  // Entry point from the fetcher
  @synchronized(self) {
    NSString *host = [[[fetcher mutableRequest] URL] host];

    if ([host length] == 0) {
#if DEBUG
      NSAssert1(0, @"%@ lacks host", fetcher);
#endif
      return YES;
    }

    NSMutableArray *runningForHost = [runningHosts_ objectForKey:host];
    if (runningForHost != nil
        && [runningForHost indexOfObjectIdenticalTo:fetcher] != NSNotFound) {
#if DEBUG
      NSAssert1(0, @"%@ was already running", fetcher);
#endif
      return YES;
    }

    // We'll save the host that serves as the key for this fetcher's array
    // to avoid any chance of the underlying request changing, stranding
    // the fetcher in the wrong array
    fetcher.serviceHost = host;
    fetcher.thread = [NSThread currentThread];

    if (maxRunningFetchersPerHost_ == 0
        || maxRunningFetchersPerHost_ > [runningForHost count]) {
      [self addRunningFetcher:fetcher forHost:host];
      return YES;
    } else {
      [self addDelayedFetcher:fetcher forHost:host];
      return NO;
    }
  }
  return YES;
}

// Fetcher start and stop methods, invoked on the appropriate thread for
// the fetcher
- (void)performSelector:(SEL)sel onStartThreadForFetcher:(GTMHTTPFetcher *)fetcher {
  NSOperationQueue *delegateQueue = fetcher.delegateQueue;
  NSThread *thread = fetcher.thread;
  if (delegateQueue != nil || [thread isEqual:[NSThread currentThread]]) {
    // The fetcher should run on the thread we're on now, or there's a delegate
    // queue specified so it doesn't matter what thread the fetcher is started
    // on, since it will call back on the queue.
    [self performSelector:sel withObject:fetcher];
  } else {
    // Fetcher must run on a specified thread (and that thread must have a
    // run loop.)
    [self performSelector:sel
                 onThread:thread
               withObject:fetcher
            waitUntilDone:NO];
  }
}

- (void)startFetcherOnCurrentThread:(GTMHTTPFetcher *)fetcher {
  [fetcher beginFetchMayDelay:NO
                 mayAuthorize:YES];
}

- (void)startFetcher:(GTMHTTPFetcher *)fetcher {
  [self performSelector:@selector(startFetcherOnCurrentThread:)
    onStartThreadForFetcher:fetcher];
}

- (void)stopFetcherOnCurrentThread:(GTMHTTPFetcher *)fetcher {
  [fetcher stopFetching];
}

- (void)stopFetcher:(GTMHTTPFetcher *)fetcher {
  [self performSelector:@selector(stopFetcherOnCurrentThread:)
    onStartThreadForFetcher:fetcher];
}

- (void)fetcherDidStop:(GTMHTTPFetcher *)fetcher {
  // Entry point from the fetcher
  @synchronized(self) {
    NSString *host = fetcher.serviceHost;
    if (!host) {
      // fetcher has been stopped previously
      return;
    }

    NSMutableArray *runningForHost = [runningHosts_ objectForKey:host];
    [runningForHost removeObject:fetcher];

    NSMutableArray *delayedForHost = [delayedHosts_ objectForKey:host];
    [delayedForHost removeObject:fetcher];

    while ([delayedForHost count] > 0
           && [runningForHost count] < maxRunningFetchersPerHost_) {
      // Start another delayed fetcher running, scanning for the minimum
      // priority value, defaulting to FIFO for equal priorities
      GTMHTTPFetcher *nextFetcher = nil;
      for (GTMHTTPFetcher *delayedFetcher in delayedForHost) {
        if (nextFetcher == nil
            || delayedFetcher.servicePriority < nextFetcher.servicePriority) {
          nextFetcher = delayedFetcher;
        }
      }

      [self addRunningFetcher:nextFetcher forHost:host];
      runningForHost = [runningHosts_ objectForKey:host];

      [delayedForHost removeObjectIdenticalTo:nextFetcher];
      [self startFetcher:nextFetcher];
    }

    if ([runningForHost count] == 0) {
      // None left; remove the empty array
      [runningHosts_ removeObjectForKey:host];
    }

    if ([delayedForHost count] == 0) {
      [delayedHosts_ removeObjectForKey:host];
    }

    // The fetcher is no longer in the running or the delayed array,
    // so remove its host and thread properties
    fetcher.serviceHost = nil;
    fetcher.thread = nil;
  }
}

- (NSUInteger)numberOfFetchers {
  @synchronized(self) {
    NSUInteger running = [self numberOfRunningFetchers];
    NSUInteger delayed = [self numberOfDelayedFetchers];
    return running + delayed;
  }
}

- (NSUInteger)numberOfRunningFetchers {
  @synchronized(self) {
    NSUInteger sum = 0;
    for (NSString *host in runningHosts_) {
      NSArray *fetchers = [runningHosts_ objectForKey:host];
      sum += [fetchers count];
    }
    return sum;
  }
}

- (NSUInteger)numberOfDelayedFetchers {
  @synchronized(self) {
    NSUInteger sum = 0;
    for (NSString *host in delayedHosts_) {
      NSArray *fetchers = [delayedHosts_ objectForKey:host];
      sum += [fetchers count];
    }
    return sum;
  }
}

- (NSArray *)issuedFetchersWithRequestURL:(NSURL *)requestURL {
  @synchronized(self) {
    NSMutableArray *array = nil;
    NSString *host = [requestURL host];
    if ([host length] == 0) return nil;

    NSURL *absRequestURL = [requestURL absoluteURL];

    NSArray *runningForHost = [runningHosts_ objectForKey:host];
    for (GTMHTTPFetcher *fetcher in runningForHost) {
      NSURL *fetcherURL = [[[fetcher mutableRequest] URL] absoluteURL];
      if ([fetcherURL isEqual:absRequestURL]) {
        if (array == nil) {
          array = [NSMutableArray array];
        }
        [array addObject:fetcher];
      }
    }

    NSArray *delayedForHost = [delayedHosts_ objectForKey:host];
    for (GTMHTTPFetcher *fetcher in delayedForHost) {
      NSURL *fetcherURL = [[[fetcher mutableRequest] URL] absoluteURL];
      if ([fetcherURL isEqual:absRequestURL]) {
        if (array == nil) {
          array = [NSMutableArray array];
        }
        [array addObject:fetcher];
      }
    }
    return array;
  }
}

- (void)stopAllFetchers {
  @synchronized(self) {
    // Remove fetchers from the delayed list to avoid fetcherDidStop: from
    // starting more fetchers running as a side effect of stopping one
    NSArray *delayedForHosts = [delayedHosts_ allValues];
    [delayedHosts_ removeAllObjects];

    for (NSArray *delayedForHost in delayedForHosts) {
      for (GTMHTTPFetcher *fetcher in delayedForHost) {
        [self stopFetcher:fetcher];
      }
    }

    NSArray *runningForHosts = [runningHosts_ allValues];
    [runningHosts_ removeAllObjects];

    for (NSArray *runningForHost in runningForHosts) {
      for (GTMHTTPFetcher *fetcher in runningForHost) {
        [self stopFetcher:fetcher];
      }
    }
  }
}

#pragma mark Fetch History Settings

// Turn on data caching to receive a copy of previously-retrieved objects.
// Otherwise, fetches may return status 304 (No Change) rather than actual data
- (void)setShouldCacheETaggedData:(BOOL)flag {
  self.fetchHistory.shouldCacheETaggedData = flag;
}

- (BOOL)shouldCacheETaggedData {
  return self.fetchHistory.shouldCacheETaggedData;
}

- (void)setETaggedDataCacheCapacity:(NSUInteger)totalBytes {
  self.fetchHistory.memoryCapacity = totalBytes;
}

- (NSUInteger)ETaggedDataCacheCapacity {
  return self.fetchHistory.memoryCapacity;
}

- (void)setShouldRememberETags:(BOOL)flag {
  self.fetchHistory.shouldRememberETags = flag;
}

- (BOOL)shouldRememberETags {
  return self.fetchHistory.shouldRememberETags;
}

// reset the ETag cache to avoid getting a Not Modified status
// based on prior queries
- (void)clearETaggedDataCache {
  [self.fetchHistory clearETaggedDataCache];
}

- (void)clearHistory {
  [self clearETaggedDataCache];
  [self.fetchHistory removeAllCookies];
}

#pragma mark Synchronous Wait for Unit Testing

- (void)waitForCompletionOfAllFetchersWithTimeout:(NSTimeInterval)timeoutInSeconds {
  NSDate* giveUpDate = [NSDate dateWithTimeIntervalSinceNow:timeoutInSeconds];
  BOOL isMainThread = [NSThread isMainThread];

  while ([self numberOfFetchers] > 0
         && [giveUpDate timeIntervalSinceNow] > 0) {
    // Run the current run loop 1/1000 of a second to give the networking
    // code a chance to work
    if (isMainThread || delegateQueue_ == nil) {
      NSDate *stopDate = [NSDate dateWithTimeIntervalSinceNow:0.001];
      [[NSRunLoop currentRunLoop] runUntilDate:stopDate];
    } else {
      // Sleep on the delegate queue's background thread.
      [NSThread sleepForTimeInterval:0.001];
    }
  }
}

#pragma mark Accessors

- (NSDictionary *)runningHosts {
  return runningHosts_;
}

- (void)setRunningHosts:(NSDictionary *)dict {
  [runningHosts_ autorelease];
  runningHosts_ = [dict mutableCopy];
}

- (NSDictionary *)delayedHosts {
  return delayedHosts_;
}

- (void)setDelayedHosts:(NSDictionary *)dict {
  [delayedHosts_ autorelease];
  delayedHosts_ = [dict mutableCopy];
}

- (id <GTMFetcherAuthorizationProtocol>)authorizer {
  return authorizer_;
}

- (void)setAuthorizer:(id <GTMFetcherAuthorizationProtocol>)obj {
  if (obj != authorizer_) {
    [self detachAuthorizer];
  }

  [authorizer_ autorelease];
  authorizer_ = [obj retain];

  // Use the fetcher service for the authorization fetches if the auth
  // object supports fetcher services
  if ([authorizer_ respondsToSelector:@selector(setFetcherService:)]) {
    [authorizer_ setFetcherService:self];
  }
}

- (void)detachAuthorizer {
  // This method is called by the fetcher service's dealloc and setAuthorizer:
  // methods; do not override.
  //
  // The fetcher service retains the authorizer, and the authorizer has a
  // weak pointer to the fetcher service (a non-zeroing pointer for
  // compatibility with iOS 4 and Mac OS X 10.5/10.6.)
  //
  // When this fetcher service no longer uses the authorizer, we want to remove
  // the authorizer's dependence on the fetcher service.  Authorizers can still
  // function without a fetcher service.
  if ([authorizer_ respondsToSelector:@selector(fetcherService)]) {
    GTMHTTPFetcherService *authFS = [authorizer_ fetcherService];
    if (authFS == self) {
      [authorizer_ setFetcherService:nil];
    }
  }
}

@end

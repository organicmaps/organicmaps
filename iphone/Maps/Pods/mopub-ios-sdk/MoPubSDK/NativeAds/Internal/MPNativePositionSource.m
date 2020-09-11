//
//  MPNativePositionSource.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativePositionSource.h"
#import "MPConstants.h"
#import "MPAdPositioning.h"
#import "MPClientAdPositioning.h"
#import "MPLogging.h"
#import "MPNativePositionResponseDeserializer.h"
#import "MPAPIEndpoints.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"
#import "MPAdServerURLBuilder.h"

static NSString * const kPositioningSourceErrorDomain = @"com.mopub.iossdk.positioningsource";
static const NSTimeInterval kMaximumRetryInterval = 60.0;
static const NSTimeInterval kMinimumRetryInterval = 1.0;
static const CGFloat kRetryIntervalBackoffMultiplier = 2.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativePositionSource ()

@property (nonatomic, assign) BOOL loading;
@property (nonatomic, copy) NSString *adUnitIdentifier;
@property (nonatomic, strong) NSURLSessionTask *task;
@property (nonatomic, copy) void (^completionHandler)(MPAdPositioning *positioning, NSError *error);
@property (nonatomic, assign) NSTimeInterval maximumRetryInterval;
@property (nonatomic, assign) NSTimeInterval minimumRetryInterval;
@property (nonatomic, assign) NSTimeInterval retryInterval;
@property (nonatomic, assign) NSUInteger retryCount;
@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPNativePositionSource

- (id)init
{
    if (self) {
        self.maximumRetryInterval = kMaximumRetryInterval;
        self.minimumRetryInterval = kMinimumRetryInterval;
        self.retryInterval = self.minimumRetryInterval;
    }
    return self;
}

- (void)dealloc
{
    [_task cancel];
}

#pragma mark - Public

- (void)loadPositionsWithAdUnitIdentifier:(NSString *)identifier completionHandler:(void (^)(MPAdPositioning *positioning, NSError *error))completionHandler
{
    NSAssert(completionHandler != nil, @"A completion handler is required to load positions.");

    if (![identifier length]) {
        NSError *invalidIDError = [NSError errorWithDomain:kPositioningSourceErrorDomain code:MPNativePositionSourceInvalidAdUnitIdentifier userInfo:nil];
        completionHandler(nil, invalidIDError);
        return;
    }

    self.adUnitIdentifier = identifier;
    self.completionHandler = completionHandler;
    self.retryCount = 0;
    self.retryInterval = self.minimumRetryInterval;

    MPLogInfo(@"Requesting ad positions for native ad unit (%@).", identifier);

    MPURLRequest *request = [[MPURLRequest alloc] initWithURL:[MPAdServerURLBuilder nativePositionUrlForAdUnitId:identifier]];
    [self.task cancel];
    self.task = [self httpTaskWithRequest:request];
}

- (void)cancel
{
    // Cancel any connection currently in flight.
    [self.task cancel];

    // Cancel any queued retry requests.
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

#pragma mark - Internal

- (NSURLSessionTask *)httpTaskWithRequest:(MPURLRequest *)request {
    __weak __typeof__(self) weakSelf = self;
    NSURLSessionTask * task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
        __typeof__(self) strongSelf = weakSelf;

        [strongSelf parsePositioningData:data];
    } errorHandler:^(NSError * _Nonnull error) {
        __typeof__(self) strongSelf = weakSelf;

        // MPNativePositionSource was deallocated during a networking
        // operation. Do nothing.
        if (strongSelf == nil) {
            return;
        }

        if (strongSelf.retryInterval >= strongSelf.maximumRetryInterval) {
            strongSelf.completionHandler(nil, error);
            strongSelf.completionHandler = nil;
        } else {
            [strongSelf performSelector:@selector(retryLoadingPositions) withObject:nil afterDelay:strongSelf.retryInterval];
            strongSelf.retryInterval = MIN(strongSelf.retryInterval * kRetryIntervalBackoffMultiplier, strongSelf.maximumRetryInterval);
        }
    }];

    return task;
}

- (void)retryLoadingPositions
{
    self.retryCount++;

    MPLogInfo(@"Retrying positions (retry attempt #%lu).", (unsigned long)self.retryCount);

    MPURLRequest *request = [[MPURLRequest alloc] initWithURL:[MPAdServerURLBuilder nativePositionUrlForAdUnitId:self.adUnitIdentifier]];
    [self.task cancel];
    self.task = [self httpTaskWithRequest:request];
}

- (void)parsePositioningData:(NSData *)data
{
    NSError *deserializationError = nil;
    MPClientAdPositioning *positioning = [[MPNativePositionResponseDeserializer deserializer] clientPositioningForData:data error:&deserializationError];

    if (deserializationError) {
        MPLogDebug(@"Position deserialization failed with error: %@", deserializationError);

        NSError *underlyingError = [[deserializationError userInfo] objectForKey:NSUnderlyingErrorKey];
        if ([underlyingError code] == MPNativePositionResponseDataIsEmpty) {
            // Empty response data means the developer hasn't assigned any ad positions for the ad
            // unit. No point in retrying the request.
            self.completionHandler(nil, [NSError errorWithDomain:kPositioningSourceErrorDomain code:MPNativePositionSourceEmptyResponse userInfo:nil]);
            self.completionHandler = nil;
        } else if (self.retryInterval >= self.maximumRetryInterval) {
            self.completionHandler(nil, deserializationError);
            self.completionHandler = nil;
        } else {
            [self performSelector:@selector(retryLoadingPositions) withObject:nil afterDelay:self.retryInterval];
            self.retryInterval = MIN(self.retryInterval * kRetryIntervalBackoffMultiplier, self.maximumRetryInterval);
        }

        return;
    }

    self.completionHandler(positioning, nil);
    self.completionHandler = nil;
}

@end

//
//  MPNativeAdSourceQueue.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAdSourceQueue.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPNativeAdRequest+MPNativeAdSource.h"
#import "MPLogging.h"
#import "MPNativeAdError.h"

static NSUInteger const kCacheSizeLimit = 1;
static NSTimeInterval const kAdFetchRetryTimes[] = {1, 3, 5, 25, 60, 300};
// Calculate the number of elements inside the array by taking the size divided by the size of one element.
static NSUInteger const kMaxRetries = sizeof(kAdFetchRetryTimes)/sizeof(kAdFetchRetryTimes[0]);

@interface MPNativeAdSourceQueue ()

@property (nonatomic) NSMutableArray *adQueue;
@property (nonatomic, assign) NSUInteger adFetchRetryCounter;
@property (nonatomic, assign) NSUInteger currentSequence;
@property (nonatomic, copy) NSString *adUnitIdentifier;
@property (nonatomic) MPNativeAdRequestTargeting *targeting;
@property (nonatomic) NSArray *rendererConfigurations;
@property (nonatomic, assign) BOOL isAdLoading;

@end

@implementation MPNativeAdSourceQueue

#pragma mark - Object Lifecycle

- (instancetype)initWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations andTargeting:(MPNativeAdRequestTargeting *)targeting
{
    self = [super init];
    if (self) {
        _adUnitIdentifier = [identifier copy];
        _rendererConfigurations = rendererConfigurations;
        _targeting = targeting;
        _adQueue = [[NSMutableArray alloc] init];
    }
    return self;
}


#pragma mark - Public Methods

- (MPNativeAd *)dequeueAd
{
    MPNativeAd *nextAd = [self.adQueue firstObject];
    [self.adQueue removeObject:nextAd];
    [self loadAds];
    return nextAd;
}

- (MPNativeAd *)dequeueAdWithMaxAge:(NSTimeInterval)age
{
    MPNativeAd *nextAd = [self dequeueAd];

    while (nextAd && ![self isAdAgeValid:nextAd withMaxAge:age]) {
        nextAd = [self dequeueAd];
    }

    return nextAd;
}

- (void)addNativeAd:(MPNativeAd *)nativeAd
{
    [self.adQueue addObject:nativeAd];
}

- (NSUInteger)count
{
    return [self.adQueue count];
}

- (void)cancelRequests
{
    [self resetBackoff];
}

#pragma mark - Internal Logic

- (BOOL)isAdAgeValid:(MPNativeAd *)ad withMaxAge:(NSTimeInterval)maxAge
{
    NSTimeInterval adAge = [ad.creationDate timeIntervalSinceNow];

    return fabs(adAge) < maxAge;
}

#pragma mark - Ad Requests

- (void)resetBackoff
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    self.adFetchRetryCounter = 0;
}

- (void)loadAds
{
    if (self.adFetchRetryCounter == 0) {
        [self replenishCache];
    }
}

- (void)replenishCache
{
    if ([self count] >= kCacheSizeLimit || self.isAdLoading) {
        return;
    }

    self.isAdLoading = YES;

    MPNativeAdRequest *adRequest = [MPNativeAdRequest requestWithAdUnitIdentifier:self.adUnitIdentifier rendererConfigurations:self.rendererConfigurations];
    adRequest.targeting = self.targeting;

    [adRequest startForAdSequence:self.currentSequence withCompletionHandler:^(MPNativeAdRequest *request, MPNativeAd *response, NSError *error) {
        if (response && !error) {
            self.adFetchRetryCounter = 0;

            [self addNativeAd:response];
            self.currentSequence++;
            if ([self count] == 1) {
                [self.delegate adSourceQueueAdIsAvailable:self];
            }
        } else {
            MPLogDebug(@"%@", error);
            //increment in this failure case to prevent retrying a request that wasn't bid on.
            //currently under discussion on whether we do this or not.
            if (error.code == MPNativeAdErrorNoInventory) {
                self.currentSequence++;
            }

            if (self.adFetchRetryCounter < kMaxRetries) {
                NSTimeInterval retryTime = kAdFetchRetryTimes[self.adFetchRetryCounter];
                self.adFetchRetryCounter++;
                [self performSelector:@selector(replenishCache) withObject:nil afterDelay:retryTime];
                MPLogDebug(@"Will re-attempt to replenish the ad cache in %.1f seconds.", retryTime);
            } else {
                // Don't try to fetch anymore ads after we have tried kMaxRetries times.
                MPLogDebug(@"Replenishing the cache has timed out.");
            }
        }
        self.isAdLoading = NO;
        [self loadAds];
    }];
}

@end

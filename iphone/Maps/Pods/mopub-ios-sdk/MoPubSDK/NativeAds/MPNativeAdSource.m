//
//  MPNativeAdSource.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdSource.h"
#import "MPNativeAd.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPNativeAdSourceQueue.h"
#import "MPConstants.h"

static NSTimeInterval const kCacheTimeoutInterval = MOPUB_ADS_EXPIRATION_INTERVAL;

@interface MPNativeAdSource () <MPNativeAdSourceQueueDelegate>

@property (nonatomic, strong) NSMutableDictionary *adQueueDictionary;

@end

@implementation MPNativeAdSource

#pragma mark - Object Lifecycle

- (instancetype)initWithDelegate:(id<MPNativeAdSourceDelegate>)delegate
{
    self = [super init];
    if (self) {
        _adQueueDictionary = [[NSMutableDictionary alloc] init];
        _delegate = delegate;
    }

    return self;
}

- (void)dealloc
{
    for (NSString *queueKey in [_adQueueDictionary allKeys]) {
        [self deleteCacheForAdUnitIdentifier:queueKey];
    }
}

#pragma mark - Ad Source Interface

- (void)loadAdsWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations andTargeting:(MPNativeAdRequestTargeting *)targeting
{
    [self deleteCacheForAdUnitIdentifier:identifier];

    MPNativeAdSourceQueue *adQueue = [[MPNativeAdSourceQueue alloc] initWithAdUnitIdentifier:identifier rendererConfigurations:rendererConfigurations andTargeting:targeting];
    adQueue.delegate = self;
    [self.adQueueDictionary setObject:adQueue forKey:identifier];

    [adQueue loadAds];
}

- (id)dequeueAdForAdUnitIdentifier:(NSString *)identifier
{
    MPNativeAdSourceQueue *adQueue = [self.adQueueDictionary objectForKey:identifier];
    MPNativeAd *nextAd = [adQueue dequeueAdWithMaxAge:kCacheTimeoutInterval];
    return nextAd;
}

- (void)deleteCacheForAdUnitIdentifier:(NSString *)identifier
{
    MPNativeAdSourceQueue *sourceQueue = [self.adQueueDictionary objectForKey:identifier];
    sourceQueue.delegate = nil;
    [sourceQueue cancelRequests];

    [self.adQueueDictionary removeObjectForKey:identifier];
}

#pragma mark - MPNativeAdSourceQueueDelegate

- (void)adSourceQueueAdIsAvailable:(MPNativeAdSourceQueue *)source
{
    [self.delegate adSourceDidFinishRequest:self];
}

@end

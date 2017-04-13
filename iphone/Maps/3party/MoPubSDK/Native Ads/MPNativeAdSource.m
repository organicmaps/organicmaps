//
//  MPNativeAdSource.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAdSource.h"
#import "MPNativeAd.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPNativeAdSourceQueue.h"

static NSTimeInterval const kCacheTimeoutInterval = 900; //15 minutes

@interface MPNativeAdSource () <MPNativeAdSourceQueueDelegate>

@property (nonatomic, strong) NSMutableDictionary *adQueueDictionary;

@end

@implementation MPNativeAdSource

#pragma mark - Object Lifecycle

+ (instancetype)source
{
    return [[MPNativeAdSource alloc] init];
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _adQueueDictionary = [[NSMutableDictionary alloc] init];
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

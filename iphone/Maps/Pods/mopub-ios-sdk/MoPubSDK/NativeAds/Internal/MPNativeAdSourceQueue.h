//
//  MPNativeAdSourceQueue.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
@class MPNativeAdRequestTargeting;
@class MPNativeAd;

@protocol MPNativeAdSourceQueueDelegate;

@interface MPNativeAdSourceQueue : NSObject

@property (nonatomic, weak) id <MPNativeAdSourceQueueDelegate> delegate;


- (instancetype)initWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations andTargeting:(MPNativeAdRequestTargeting *)targeting;
- (MPNativeAd *)dequeueAdWithMaxAge:(NSTimeInterval)age;
- (NSUInteger)count;
- (void)loadAds;
- (void)cancelRequests;

@end

@protocol MPNativeAdSourceQueueDelegate <NSObject>

- (void)adSourceQueueAdIsAvailable:(MPNativeAdSourceQueue *)source;

@end

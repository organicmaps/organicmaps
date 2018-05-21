//
//  MPNativeAdSource.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPNativeAdSourceDelegate.h"
@class MPNativeAdRequestTargeting;

@interface MPNativeAdSource : NSObject

@property (nonatomic, weak) id <MPNativeAdSourceDelegate> delegate;

+ (instancetype)source;
- (void)loadAdsWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations andTargeting:(MPNativeAdRequestTargeting *)targeting;
- (void)deleteCacheForAdUnitIdentifier:(NSString *)identifier;
- (id)dequeueAdForAdUnitIdentifier:(NSString *)identifier;


@end

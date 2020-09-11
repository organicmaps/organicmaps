//
//  MPNativeAdSource.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPNativeAdSourceDelegate.h"
@class MPNativeAdRequestTargeting;

@interface MPNativeAdSource : NSObject

@property (nonatomic, weak) id<MPNativeAdSourceDelegate> delegate;

- (instancetype)initWithDelegate:(id<MPNativeAdSourceDelegate>)delegate;

- (void)loadAdsWithAdUnitIdentifier:(NSString *)identifier rendererConfigurations:(NSArray *)rendererConfigurations andTargeting:(MPNativeAdRequestTargeting *)targeting;
- (void)deleteCacheForAdUnitIdentifier:(NSString *)identifier;
- (id)dequeueAdForAdUnitIdentifier:(NSString *)identifier;

@end

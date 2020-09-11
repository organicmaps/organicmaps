//
//  MPPrivateBannerCustomEventDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPBannerCustomEventDelegate.h"

@class MPAdConfiguration;

@protocol MPPrivateBannerCustomEventDelegate <MPBannerCustomEventDelegate>

- (NSString *)adUnitId;
- (MPAdConfiguration *)configuration;
- (id)bannerDelegate;

@end

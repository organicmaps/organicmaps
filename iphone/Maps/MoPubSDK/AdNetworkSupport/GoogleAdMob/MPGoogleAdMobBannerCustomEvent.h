//
//  MPGoogleAdMobBannerCustomEvent.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPBannerCustomEvent.h"
#import "GADBannerView.h"

/*
 * Compatible with version 6.2.0 of the Google AdMob Ads SDK.
 */

@interface MPGoogleAdMobBannerCustomEvent : MPBannerCustomEvent <GADBannerViewDelegate>

@end

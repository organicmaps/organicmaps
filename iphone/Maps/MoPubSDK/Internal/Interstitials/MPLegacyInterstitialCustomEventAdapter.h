//
//  MPLegacyInterstitialCustomEventAdapter.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPBaseInterstitialAdapter.h"

@interface MPLegacyInterstitialCustomEventAdapter : MPBaseInterstitialAdapter

- (void)customEventDidLoadAd;
- (void)customEventDidFailToLoadAd;
- (void)customEventActionWillBegin;

@end

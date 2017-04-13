//
//  MPBannerCustomEventAdapter.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPBaseBannerAdapter.h"

#import "MPPrivateBannerCustomEventDelegate.h"

@class MPBannerCustomEvent;

@interface MPBannerCustomEventAdapter : MPBaseBannerAdapter <MPPrivateBannerCustomEventDelegate>

@end

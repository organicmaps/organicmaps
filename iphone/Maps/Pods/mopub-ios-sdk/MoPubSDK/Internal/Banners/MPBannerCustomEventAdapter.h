//
//  MPBannerCustomEventAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseBannerAdapter.h"

#import "MPPrivateBannerCustomEventDelegate.h"

@class MPBannerCustomEvent;

@interface MPBannerCustomEventAdapter : MPBaseBannerAdapter <MPPrivateBannerCustomEventDelegate>

- (instancetype)initWithConfiguration:(MPAdConfiguration *)configuration delegate:(id<MPBannerAdapterDelegate>)delegate;

@end

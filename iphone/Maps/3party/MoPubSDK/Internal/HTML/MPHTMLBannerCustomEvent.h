//
//  MPHTMLBannerCustomEvent.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPBannerCustomEvent.h"
#import "MPAdWebViewAgent.h"
#import "MPPrivateBannerCustomEventDelegate.h"

@interface MPHTMLBannerCustomEvent : MPBannerCustomEvent <MPAdWebViewAgentDelegate>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-property-synthesis"
@property (nonatomic, weak) id<MPPrivateBannerCustomEventDelegate> delegate;
#pragma clang diagnostic pop

@end

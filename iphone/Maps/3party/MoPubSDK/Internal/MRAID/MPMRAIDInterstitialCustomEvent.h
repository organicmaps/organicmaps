//
//  MPMRAIDInterstitialCustomEvent.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPInterstitialCustomEvent.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPPrivateInterstitialCustomEventDelegate.h"

@interface MPMRAIDInterstitialCustomEvent : MPInterstitialCustomEvent <MPInterstitialViewControllerDelegate>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-property-synthesis"
@property (nonatomic, weak) id<MPPrivateInterstitialCustomEventDelegate> delegate;
#pragma clang diagnostic pop

@end

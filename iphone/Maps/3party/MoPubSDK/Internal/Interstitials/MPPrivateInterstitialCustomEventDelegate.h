//
//  MPPrivateInterstitialcustomEventDelegate.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPInterstitialCustomEventDelegate.h"

@class MPAdConfiguration;
@class CLLocation;

@protocol MPPrivateInterstitialCustomEventDelegate <MPInterstitialCustomEventDelegate>

- (NSString *)adUnitId;
- (MPAdConfiguration *)configuration;
- (id)interstitialDelegate;

@end

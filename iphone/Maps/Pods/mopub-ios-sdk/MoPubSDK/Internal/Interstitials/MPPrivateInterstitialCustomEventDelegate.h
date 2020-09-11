//
//  MPPrivateInterstitialCustomEventDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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

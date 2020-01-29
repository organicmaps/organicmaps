//
// Created by Moat on 2/24/15.
// Copyright © 2016 Moat. All rights reserved.
//
// This class is used to track native ads -- ads that are rendered using native UI elements rather than a WebView.
// The class creates an internally managed WebView instance, loads our JavaScript tag into it, and then dispatches
// viewability-related signals (pertaining to the native ad it is tracking) into that WebView.

#import <UIKit/UIKit.h>
#import "MPUBMoatBaseTracker.h"

@interface MPUBMoatNativeDisplayTracker : MPUBMoatBaseTracker

// Use this to track ads that can't run JavaScript. This method accepts any UIView.
// Web-based ads, including "opaque" web containers (Google's DFPBannerView, etc.) are best tracked using MoatWebTracker instead.
+ (MPUBMoatNativeDisplayTracker *)trackerWithAdView:(UIView *)adView withAdIds:(NSDictionary *)adIds;

- (void)startTracking;

// Call to stop tracking the current ad.
- (void)stopTracking;

@end

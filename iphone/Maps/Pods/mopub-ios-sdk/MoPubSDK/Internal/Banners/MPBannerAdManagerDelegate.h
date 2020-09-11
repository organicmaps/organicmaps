//
//  MPBannerAdManagerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPAdView;
@class MPAdTargeting;

@protocol MPAdViewDelegate;

@protocol MPBannerAdManagerDelegate <NSObject>

- (NSString *)adUnitId;
- (MPNativeAdOrientation)allowedNativeAdsOrientation;
- (MPAdView *)banner;
- (id<MPAdViewDelegate>)bannerDelegate;
- (CGSize)containerSize;
- (UIViewController *)viewControllerForPresentingModalView;

/**
 * The latest ad targeting information for ad refresh and other scenarios.
 */
- (MPAdTargeting *)adTargeting;

- (void)invalidateContentView;

- (void)managerDidLoadAd:(UIView *)ad;
- (void)managerDidFailToLoadAdWithError:(NSError *)error;
- (void)userActionWillBegin;
- (void)userActionDidFinish;
- (void)userWillLeaveApplication;
- (void)impressionDidFireWithImpressionData:(MPImpressionData *)impressionData;

@end

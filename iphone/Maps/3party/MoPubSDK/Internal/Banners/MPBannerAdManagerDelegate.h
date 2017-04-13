//
//  MPBannerAdManagerDelegate.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MPAdView;
@protocol MPAdViewDelegate;

@protocol MPBannerAdManagerDelegate <NSObject>

- (NSString *)adUnitId;
- (MPNativeAdOrientation)allowedNativeAdsOrientation;
- (MPAdView *)banner;
- (id<MPAdViewDelegate>)bannerDelegate;
- (CGSize)containerSize;
- (NSString *)keywords;
- (CLLocation *)location;
- (BOOL)isTesting;
- (UIViewController *)viewControllerForPresentingModalView;

- (void)invalidateContentView;

- (void)managerDidLoadAd:(UIView *)ad;
- (void)managerDidFailToLoadAd;
- (void)userActionWillBegin;
- (void)userActionDidFinish;
- (void)userWillLeaveApplication;

@end

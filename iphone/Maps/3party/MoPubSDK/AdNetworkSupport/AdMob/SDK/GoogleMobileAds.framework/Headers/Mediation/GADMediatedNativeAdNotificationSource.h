//
//  GADMediatedNativeAdNotificationSource.h
//  Google Mobile Ads SDK
//
//  Copyright 2015 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <GoogleMobileAds/GoogleMobileAdsDefines.h>
#import <GoogleMobileAds/Mediation/GADMediatedNativeAd.h>

GAD_ASSUME_NONNULL_BEGIN

/// Used by mediation adapters to notify the Google Mobile Ads SDK about events occurring in the
/// lifecycle of a GADMediatedNativeAd.
@interface GADMediatedNativeAdNotificationSource : NSObject

/// Called by the adapter when it has registered an impression on the tracked view. Adapter should
/// only call this method if -[GADMAdNetworkAdapter handlesUserImpressions] returns YES.
+ (void)mediatedNativeAdDidRecordImpression:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Called by the adapter when it has registered a user click on the tracked view. Adapter should
/// only call this method if -[GADMAdNetworkAdapter handlesUserClicks] returns YES.
+ (void)mediatedNativeAdDidRecordClick:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before mediatedNativeAd has opened an in-app modal screen.
+ (void)mediatedNativeAdWillPresentScreen:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before the in app modal screen opened by mediatedNativeAd is
/// dismissed.
+ (void)mediatedNativeAdWillDismissScreen:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter after the in app modal screen opened by mediatedNativeAd is
/// dismissed.
+ (void)mediatedNativeAdDidDismissScreen:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before mediatedNativeAd causes another app (such as a browser
/// or the App Store) to take input focus.
+ (void)mediatedNativeAdWillLeaveApplication:(id<GADMediatedNativeAd>)mediatedNativeAd;

#pragma mark - Mediated Native Video Ad Notifications

/// Called by the adapter when native video playback has begun or resumed.
+ (void)mediatedNativeAdDidPlayVideo:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Called by the adapter when native video playback has paused.
+ (void)mediatedNativeAdDidPauseVideo:(id<GADMediatedNativeAd>)mediatedNativeAd;

/// Called by the adapter when native video playback has ended.
+ (void)mediatedNativeAdDidEndVideoPlayback:(id<GADMediatedNativeAd>)mediatedNativeAd;

@end

GAD_ASSUME_NONNULL_END

//
//  GADMediatedUnifiedNativeAdNotificationSource.h
//  Google Mobile Ads SDK
//
//  Copyright 2017 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <GoogleMobileAds/GoogleMobileAdsDefines.h>
#import <GoogleMobileAds/Mediation/GADMediatedUnifiedNativeAd.h>

GAD_ASSUME_NONNULL_BEGIN

/// Used by mediation adapters to notify the Google Mobile Ads SDK about events occurring in the
/// lifecycle of a GADMediatedUnifiedNativeAd.
@interface GADMediatedUnifiedNativeAdNotificationSource : NSObject

/// Called by the adapter when it has registered an impression on the tracked view. Adapter should
/// only call this method if -[GADMAdNetworkAdapter handlesUserImpressions] returns YES.
+ (void)mediatedNativeAdDidRecordImpression:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Called by the adapter when it has registered a user click on the tracked view. Adapter should
/// only call this method if -[GADMAdNetworkAdapter handlesUserClicks] returns YES.
+ (void)mediatedNativeAdDidRecordClick:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before mediatedNativeAd has opened an in-app modal screen.
+ (void)mediatedNativeAdWillPresentScreen:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before the in app modal screen opened by mediatedNativeAd is
/// dismissed.
+ (void)mediatedNativeAdWillDismissScreen:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter after the in app modal screen opened by mediatedNativeAd is
/// dismissed.
+ (void)mediatedNativeAdDidDismissScreen:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Must be called by the adapter just before mediatedNativeAd causes another app (such as a browser
/// or the App Store) to take input focus.
+ (void)mediatedNativeAdWillLeaveApplication:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

#pragma mark - Mediated Native Video Ad Notifications

/// Called by the adapter when native video playback has begun or resumed.
+ (void)mediatedNativeAdDidPlayVideo:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Called by the adapter when native video playback has paused.
+ (void)mediatedNativeAdDidPauseVideo:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

/// Called by the adapter when native video playback has ended.
+ (void)mediatedNativeAdDidEndVideoPlayback:(id<GADMediatedUnifiedNativeAd>)mediatedNativeAd;

@end

GAD_ASSUME_NONNULL_END

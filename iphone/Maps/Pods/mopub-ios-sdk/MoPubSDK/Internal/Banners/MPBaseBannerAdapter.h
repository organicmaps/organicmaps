//
//  MPBaseBannerAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPAdView.h"

@protocol MPBannerAdapterDelegate;
@class MPAdConfiguration;
@class MPAdTargeting;

@interface MPBaseBannerAdapter : NSObject

@property (nonatomic, weak) id<MPBannerAdapterDelegate> delegate;
@property (nonatomic, copy) NSURL *impressionTrackingURL;
@property (nonatomic, copy) NSURL *clickTrackingURL;

- (instancetype)initWithDelegate:(id<MPBannerAdapterDelegate>)delegate;

/*
 * Sets the adapter's delegate to nil.
 */
- (void)unregisterDelegate;

/*
 * -_getAdWithConfiguration creates a strong reference to self before calling
 * -getAdWithConfiguration to prevent the adapter from being prematurely deallocated.
 */
- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting containerSize:(CGSize)size;
- (void)_getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting containerSize:(CGSize)size;

- (void)didStopLoading;
- (void)didDisplayAd;

/*
 * Your subclass should implement this method if your native ads vary depending on orientation.
 */
- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation;

- (void)trackImpression;

- (void)trackClick;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPBannerAdapterDelegate

@required

- (MPAdView *)banner;
- (id<MPAdViewDelegate>)bannerDelegate;
- (UIViewController *)viewControllerForPresentingModalView;
- (MPNativeAdOrientation)allowedNativeAdsOrientation;
- (CLLocation *)location;

/*
 * These callbacks notify you that the adapter (un)successfully loaded an ad.
 */
- (void)adapter:(MPBaseBannerAdapter *)adapter didFailToLoadAdWithError:(NSError *)error;
- (void)adapter:(MPBaseBannerAdapter *)adapter didFinishLoadingAd:(UIView *)ad;

/*
 * These callbacks notify you that the user interacted (or stopped interacting) with the native ad.
 */
- (void)userActionWillBeginForAdapter:(MPBaseBannerAdapter *)adapter;
- (void)userActionDidFinishForAdapter:(MPBaseBannerAdapter *)adapter;

/*
 * This callback notifies you that user has tapped on an ad which will cause them to leave the
 * current application (e.g. the ad action opens the iTunes store, Mobile Safari, etc).
 */
- (void)userWillLeaveApplicationFromAdapter:(MPBaseBannerAdapter *)adapter;

/**
 * Fires when the impression tracker has been sent.
 */
- (void)adapterDidTrackImpressionForAd:(MPBaseBannerAdapter *)adapter;

/**
 * Fires when the banner ad is expanding/resizing and collapsing.
 */
- (void)adWillExpandForAdapter:(MPBaseBannerAdapter *)adapter;
- (void)adDidCollapseForAdapter:(MPBaseBannerAdapter *)adapter;

@end

//
//  MPBannerCustomEventDelegate.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

@class MPBannerCustomEvent;

/**
 * Instances of your custom subclass of `MPBannerCustomEvent` will have an `MPBannerCustomEventDelegate` delegate.
 * You use this delegate to communicate events ad events back to the MoPub SDK.
 *
 * When mediating a third party ad network it is important to call as many of these methods
 * as accurately as possible.  Not all ad networks support all these events, and some support
 * different events.  It is your responsibility to find an appropriate mapping betwen the ad
 * network's events and the callbacks defined on `MPBannerCustomEventDelegate`.
 */

@protocol MPBannerCustomEventDelegate <NSObject>

/**
 * The view controller instance to use when presenting modals.
 *
 * @return `viewControllerForPresentingModalView` returns the same view controller that you
 * specify when implementing the `MPAdViewDelegate` protocol.
 */
- (UIViewController *)viewControllerForPresentingModalView;

/**
 * The user's current location.
 *
 * @return This method provides the location that was passed into the parent `MPAdView`.  The MoPub
 * SDK does **not** automatically request the user's location.  It is your responsibility to pass the location
 * into `MPAdView`.
 *
 * You may use this to inform third-party ad networks of the user's location.
 */
- (CLLocation *)location;

/** @name Banner Ad Event Callbacks - Fetching Ads */

/**
 * Call this method immediately after an ad loads succesfully.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param ad The `UIView` representing the banner ad.  This view will be inserted into the `MPAdView`
 * and presented to the user by the MoPub SDK.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it successfully loads an ad.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)bannerCustomEvent:(MPBannerCustomEvent *)event didLoadAd:(UIView *)ad;

/**
 * Call this method immediately after an ad fails to load.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param error (*optional*) You may pass an error describing the failure.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it fails to load an ad.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)bannerCustomEvent:(MPBannerCustomEvent *)event didFailToLoadAdWithError:(NSError *)error;

/** @name Banner Ad Event Callbacks - User Interaction */

/**
 * Call this method when the user taps on the banner ad.
 *
 * This method is optional.  When automatic click and impression tracking is enabled (the default)
 * this method will track a click (the click is guaranteed to only be tracked once per ad).
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: If you call `-bannerCustomEventWillBeginAction:`, you _**must**_ also call
 * `-bannerCustomEventDidFinishAction:` at a later point.
 */
- (void)bannerCustomEventWillBeginAction:(MPBannerCustomEvent *)event;

/**
 * Call this method when the user finishes interacting with the banner ad.
 *
 * For example, the user may have dismissed any modal content. This method is optional.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: If you call `-bannerCustomEventWillBeginAction:`, you _**must**_ also call
 * `-bannerCustomEventDidFinishAction:` at a later point.
 */
- (void)bannerCustomEventDidFinishAction:(MPBannerCustomEvent *)event;

/**
 * Call this method when the banner ad will cause the user to leave the application.
 *
 * For example, the user may have tapped on a link to visit the App Store or Safari.
 *
 * @param event You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 */
- (void)bannerCustomEventWillLeaveApplication:(MPBannerCustomEvent *)event;

/** @name Impression and Click Tracking */

/**
 * Call this method to track an impression.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPBannerCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks impressions.
 *
 * **Important**: In order to obtain accurate metrics, it is your responsibility to call `trackImpression` only **once** per ad.
 */
- (void)trackImpression;

/**
 * Call this method to track a click.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPBannerCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks clicks.
 *
 * **Important**: In order to obtain accurate metrics, it is your responsibility to call `trackClick` only **once** per ad.
 */
- (void)trackClick;

@end

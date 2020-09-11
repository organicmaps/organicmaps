//
//  MPInterstitialCustomEventDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@class MPInterstitialCustomEvent;

/**
 * Instances of your custom subclass of `MPInterstitialCustomEvent` will have an `MPInterstitialCustomEventDelegate` delegate.
 * You use this delegate to communicate events ad events back to the MoPub SDK.
 *
 * When mediating a third party ad network it is important to call as many of these methods
 * as accurately as possible.  Not all ad networks support all these events, and some support
 * different events.  It is your responsibility to find an appropriate mapping betwen the ad
 * network's events and the callbacks defined on `MPInterstitialCustomEventDelegate`.
 */

@protocol MPInterstitialCustomEventDelegate <NSObject>

/**
 * The user's current location.
 *
 * @return This method provides the location that was passed into the parent `MPInterstitialAdController`.  The MoPub
 * SDK does **not** automatically request the user's location.  It is your responsibility to pass the location
 * into `MPInterstitialAdController`.
 *
 * You may use this to inform third-party ad networks of the user's location.
 */
- (CLLocation *)location;

/** @name Interstitial Ad Event Callbacks - Fetching Ads */

/**
 * Call this method immediately after an ad loads succesfully.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param ad (*optional*) An object that represents the ad that was retrieved.  The MoPub SDK does not
 * do anything with this optional parameter.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it successfully loads an ad.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)interstitialCustomEvent:(MPInterstitialCustomEvent *)customEvent
                      didLoadAd:(id)ad;

/**
 * Call this method immediately after an ad fails to load.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param error (*optional*) You may pass an error describing the failure.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it fails to load an ad.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)interstitialCustomEvent:(MPInterstitialCustomEvent *)customEvent
       didFailToLoadAdWithError:(NSError *)error;

/**
 * Call this method if a previously loaded interstitial should no longer be eligible for presentation.
 *
 * Some third-party networks will mark interstitials as expired (indicating they should not be
 * presented) *after* they have loaded.  You may use this method to inform the MoPub SDK that a
 * previously loaded interstitial has expired and that a new interstitial should be obtained.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 */
- (void)interstitialCustomEventDidExpire:(MPInterstitialCustomEvent *)customEvent;

/** @name Interstitial Ad Event Callbacks - Presenting and Dismissing Ads */

/**
 * Call this method when an ad is about to appear.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is about to present the interstitial.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 */
- (void)interstitialCustomEventWillAppear:(MPInterstitialCustomEvent *)customEvent;

/**
 * Call this method when an ad has finished appearing.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is finished presenting the interstitial.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 * **Note**: if it is not possible to know when the interstitial *finished* appearing, you should call
 * this immediately after calling `-interstitialCustomEventWillAppear:`.
 */
- (void)interstitialCustomEventDidAppear:(MPInterstitialCustomEvent *)customEvent;

/**
 * Call this method when an ad is about to disappear.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is about to dismiss the interstitial.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 */
- (void)interstitialCustomEventWillDisappear:(MPInterstitialCustomEvent *)customEvent;

/**
 * Call this method when an ad has finished disappearing.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is finished with dismissing the interstitial.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 * **Note**: if it is not possible to know when the interstitial *finished* dismissing, you should call
 * this immediately after calling `-interstitialCustomEventDidDisappear:`.
 */
- (void)interstitialCustomEventDidDisappear:(MPInterstitialCustomEvent *)customEvent;

/** @name Interstitial Ad Event Callbacks - User Interaction */

/**
 * Call this method when the user taps on the interstitial ad.
 *
 * This method is optional.  When automatic click and impression tracking is enabled (the default)
 * this method will track a click (the click is guaranteed to only be tracked once per ad).
 *
 * **Note**: some third-party networks provide a "will leave application" callback instead of/in
 * addition to a "user did click" callback. You should call this method in response to either of
 * those callbacks (since leaving the application is generally an indicator of a user tap).
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 */
- (void)interstitialCustomEventDidReceiveTapEvent:(MPInterstitialCustomEvent *)customEvent;

/**
 * Call this method when the interstitial ad will cause the user to leave the application.
 *
 * For example, the user may have tapped on a link to visit the App Store or Safari.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 */
- (void)interstitialCustomEventWillLeaveApplication:(MPInterstitialCustomEvent *)customEvent;

/** @name Impression and Click Tracking */

/**
 * Call this method to track an impression.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPInterstitialCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks impressions.
 *
 * **Important**: In order to obtain accurate metrics, it is your responsibility to call `trackImpression` only **once** per ad.
 */
- (void)trackImpression;

/**
 * Call this method to track a click.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPInterstitialCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks clicks.
 *
 * **Important**: In order to obtain accurate metrics, it is your responsibility to call `trackClick` only **once** per ad.
 */
- (void)trackClick;

@end

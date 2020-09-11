//
//  MPInterstitialAdControllerDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubAd.h"

@class MPInterstitialAdController;

/**
 * The delegate of an `MPInterstitialAdController` object must adopt the
 * `MPInterstitialAdControllerDelegate` protocol.
 *
 * The optional methods of this protocol allow the delegate to be notified of interstitial state
 * changes, such as when an ad has loaded, when an ad has been presented or dismissed from the
 * screen, and when an ad has expired.
 */

@protocol MPInterstitialAdControllerDelegate <MPMoPubAdDelegate>

@optional

/** @name Detecting When an Interstitial Ad is Loaded */

/**
 * Sent when an interstitial ad object successfully loads an ad.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidLoadAd:(MPInterstitialAdController *)interstitial;

/**
 * Sent when an interstitial ad object fails to load an ad.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidFailToLoadAd:(MPInterstitialAdController *)interstitial;

/**
 * Sent when an interstitial ad object fails to load an ad.
 *
 * @param interstitial The interstitial ad object sending the message.
 * @param error The error that occurred during the load.
 */
- (void)interstitialDidFailToLoadAd:(MPInterstitialAdController *)interstitial
                          withError:(NSError *)error;

/** @name Detecting When an Interstitial Ad is Presented */

/**
 * Sent immediately before an interstitial ad object is presented on the screen.
 *
 * Your implementation of this method should pause any application activity that requires user
 * interaction.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialWillAppear:(MPInterstitialAdController *)interstitial;

/**
 * Sent after an interstitial ad object has been presented on the screen.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidAppear:(MPInterstitialAdController *)interstitial;

/** @name Detecting When an Interstitial Ad is Dismissed */

/**
 * Sent immediately before an interstitial ad object will be dismissed from the screen.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialWillDisappear:(MPInterstitialAdController *)interstitial;

/**
 * Sent after an interstitial ad object has been dismissed from the screen, returning control
 * to your application.
 *
 * Your implementation of this method should resume any application activity that was paused
 * prior to the interstitial being presented on-screen.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidDisappear:(MPInterstitialAdController *)interstitial;

/** @name Detecting When an Interstitial Ad Expires */

/**
 * Sent when a loaded interstitial ad is no longer eligible to be displayed.
 *
 * Interstitial ads from certain networks may expire their content at any time,
 * even if the content is currently on-screen. This method notifies you when the currently-
 * loaded interstitial has expired and is no longer eligible for display.
 *
 * If the ad was on-screen when it expired, you can expect that the ad will already have been
 * dismissed by the time this message is sent.
 *
 * Your implementation may include a call to `loadAd` to fetch a new ad, if desired.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidExpire:(MPInterstitialAdController *)interstitial;

/**
 * Sent when the user taps the interstitial ad and the ad is about to perform its target action.
 *
 * This action may include displaying a modal or leaving your application. Certain ad networks
 * may not expose a "tapped" callback so you should not rely on this callback to perform
 * critical tasks.
 *
 * @param interstitial The interstitial ad object sending the message.
 */
- (void)interstitialDidReceiveTapEvent:(MPInterstitialAdController *)interstitial;

@end

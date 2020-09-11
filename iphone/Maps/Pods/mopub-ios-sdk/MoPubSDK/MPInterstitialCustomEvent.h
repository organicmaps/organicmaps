//
//  MPInterstitialCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPInterstitialCustomEventDelegate.h"

/**
 * The MoPub iOS SDK mediates third party Ad Networks using custom events.  The custom events are
 * responsible for instantiating and manipulating objects in the third party SDK and translating
 * and communicating events from those objects back to the MoPub SDK by notifying a delegate.
 *
 * `MPInterstitialCustomEvent` is a base class for custom events that support full-screen interstitial ads.
 * By implementing subclasses of `MPInterstitialCustomEvent` you can enable the MoPub SDK to
 * natively support a wide variety of third-party ad networks.
 *
 * At runtime, the MoPub SDK will find and instantiate an `MPInterstitialCustomEvent` subclass as needed and
 * invoke its `-requestInterstitialWithCustomEventInfo:` method.
 */


@interface MPInterstitialCustomEvent : NSObject

/** @name Requesting and Displaying an Interstitial Ad */

/**
 * Called when the MoPub SDK requires a new interstitial ad.
 *
 * When the MoPub SDK receives a response indicating it should load a custom event, it will send
 * this message to your custom event class. Your implementation of this method should load an
 * interstitial ad from a third-party ad network. It must also notify the
 * `MPInterstitialCustomEventDelegate` of certain lifecycle events.
 *
 * @param info A  dictionary containing additional custom data associated with a given custom event
 * request. This data is configurable on the MoPub website, and may be used to pass dynamic information, such as publisher IDs.
 * @param adMarkup An optional ad markup to use.
 */

- (void)requestInterstitialWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup;

/**
 * Called when the interstitial should be displayed.
 *
 * This message is sent sometime after an interstitial has been successfully loaded, as a result
 * of your code calling `-[MPInterstitialAdController showFromViewController:]`. Your implementation
 * of this method should present the interstitial ad from the specified view controller.
 *
 * If you decide to [opt out of automatic impression tracking](enableAutomaticImpressionAndClickTracking), you should place your
 * manual calls to [-trackImpression]([MPInterstitialCustomEventDelegate trackImpression]) in this method to ensure correct metrics.
 *
 * @param rootViewController The controller to use to present the interstitial modally.
 *
 */
- (void)showInterstitialFromRootViewController:(UIViewController *)rootViewController;

/** @name Impression and Click Tracking */

/**
 * Override to opt out of automatic impression and click tracking.
 *
 * By default, the  MPInterstitialCustomEventDelegate will automatically record impressions and clicks in
 * response to the appropriate callbacks. You may override this behavior by implementing this method
 * to return `NO`.
 *
 * @warning **Important**: If you do this, you are responsible for calling the `[-trackImpression]([MPInterstitialCustomEventDelegate trackImpression])` and
 * `[-trackClick]([MPInterstitialCustomEventDelegate trackClick])` methods on the custom event delegate. Additionally, you should make sure that these
 * methods are only called **once** per ad.
 */
- (BOOL)enableAutomaticImpressionAndClickTracking;

/** @name Communicating with the MoPub SDK */

/**
 * The `MPInterstitialCustomEventDelegate` to send messages to as events occur.
 *
 * The `delegate` object defines several methods that you should call in order to inform both MoPub
 * and your `MPInterstitialAdController`'s delegate of the progress of your custom event.
 *
 */

@property (nonatomic, weak) id<MPInterstitialCustomEventDelegate> delegate;

/**
 * An optional dictionary containing extra local data.
 */
@property (nonatomic, copy) NSDictionary *localExtras;

@end

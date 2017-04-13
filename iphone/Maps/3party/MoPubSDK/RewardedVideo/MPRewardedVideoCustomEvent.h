//
//  MPRewardedVideoCustomEvent.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol MPRewardedVideoCustomEventDelegate;
@protocol MPMediationSettingsProtocol;

@class MPRewardedVideoReward;

/**
 * The MoPub iOS SDK mediates third party Ad Networks using custom events.  The custom events are
 * responsible for instantiating and manipulating objects in the third party SDK and translating
 * and communicating events from those objects back to the MoPub SDK by notifying a delegate.
 *
 * `MPRewardedVideoCustomEvent` is a base class for custom events that support full-screen rewarded video ads.
 * By implementing subclasses of `MPRewardedVideoCustomEvent` you can enable the MoPub SDK to
 * natively support a wide variety of third-party ad networks.
 *
 * At runtime, the MoPub SDK will find and instantiate an `MPRewardedVideoCustomEvent` subclass as needed and
 * invoke its `-requestRewardedVideoWithCustomEventInfo:` method.
 */

@interface MPRewardedVideoCustomEvent : NSObject

@property (nonatomic, weak) id<MPRewardedVideoCustomEventDelegate> delegate;

/** @name Requesting and Displaying a Rewarded Video Ad */

/**
 * Called when the MoPub SDK requires a new rewarded video ad.
 *
 * When the MoPub SDK receives a response indicating it should load a custom event, it will send
 * this message to your custom event class. Your implementation of this method should load an
 * rewarded video ad from a third-party ad network. It must also notify the
 * `MPRewardedVideoCustomEventDelegate` of certain lifecycle events.
 *
 * The default implementation of this method does nothing. Subclasses must override this method and implement code to load a rewarded video here.
 *
 * **Important**: The application may provide a mediation settings object containing properties that you should use to configure how you use
 * the ad network's APIs. Call `[-mediationSettingsForClass:]([MPRewardedVideoCustomEventDelegate mediationSettingsForClass:])`
 * specifying a specific class that your custom event uses to retrieve the mediation settings object if it exists. You define
 * the mediation settings class and the properties it supports for your custom event.
 *
 * @param info A dictionary containing additional custom data associated with a given custom event
 * request. This data is configurable on the MoPub website, and may be used to pass dynamic information, such as publisher IDs.
 */
- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info;

/**
 * Called when the MoPubSDK wants to know if an ad is currently available for the ad network.
 *
 * This call is typically invoked when the application wants to check whether an ad unit has an ad ready to display.
 *
 * Subclasses must override this method and implement coheck whether or not a rewarded vidoe ad is available for presentation.
 *
 */
- (BOOL)hasAdAvailable;

/**
 * Called when the rewarded video should be displayed.
 *
 * This message is sent sometime after a rewarded video has been successfully loaded, as a result
 * of your code calling `-[MPRewardedVideo presentRewardedVideoAdForAdUnitID:fromViewController:]`. Your implementation
 * of this method should present the rewarded video ad from the specified view controller.
 *
 * The default implementation of this method does nothing. Subclasses must override this method and implement code to display a rewarded video here.
 *
 * If you decide to [opt out of automatic impression tracking](enableAutomaticImpressionAndClickTracking), you should place your
 * manual calls to [-trackImpression]([MPRewardedVideoCustomEventDelegate trackImpression]) in this method to ensure correct metrics.
 *
 * @param viewController The controller to use to present the rewarded video modally.
 */
- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController;

/** @name Impression and Click Tracking */

/**
 * Override to opt out of automatic impression and click tracking.
 *
 * By default, the  MPRewardedVideoCustomEventDelegate will automatically record impressions and clicks in
 * response to the appropriate callbacks. You may override this behavior by implementing this method
 * to return `NO`.
 *
 * @warning **Important**: If you do this, you are responsible for calling the `[-trackImpression]([MPRewardedVideoCustomEventDelegate trackImpression])` and
 * `[-trackClick]([MPRewardedVideoCustomEventDelegate trackClick])` methods on the custom event delegate. Additionally, you should make sure that these
 * methods are only called **once** per ad.
 */
- (BOOL)enableAutomaticImpressionAndClickTracking;

/**
 * Override this method to handle when an ad was played for this custom event's network, but under a different ad unit ID.
 *
 * Due to the way ad mediation works, two ad units may load the same ad network for displaying ads. When one ad unit plays
 * an ad, the other ad unit may need to update its state and notify the application an ad may no longer be available as it
 * may have already played. If an ad becomes unavailable for this custom event, call
 * `[-rewardedVideoDidExpireForCustomEvent:]([MPRewardedVideoCustomEventDelegate rewardedVideoDidExpireForCustomEvent:])`
 * to notify the application that an ad is no longer available.
 *
 * This method will only be called if your custom event has reported that an ad had successfully loaded. The default implementation of this method does nothing.
 * Subclasses must override this method and implement code to handle when the custom event is no longer needed by the rewarded video system.
 */
- (void)handleAdPlayedForCustomEventNetwork;

/**
 * Override this method to handle when the custom event is no longer needed by the rewarded video system.
 *
 * This method is called once the rewarded video system no longer references your custom event. This method
 * is provided as you may have a centralized object holding onto this custom event. If that is the case and your
 * centralized object no longer needs the custom event, then you should remove the custom event from the centralized
 * object in this method causing the custom event to deallocate. See `MPAdColonyRewardedVideoCustomEvent` for an
 * example of how and why this method is used.
 *
 * Implementation of this method is not necessary if you do not hold any extra references to it. `-dealloc` will still
 * be called. However, it is expected you will need to override this method to prevent memory leaks. It is safe to override with nothing
 * if you believe you will not leak memory.
 */
- (void)handleCustomEventInvalidated;

@end

@protocol MPRewardedVideoCustomEventDelegate <NSObject>

/** @name Rewarded Video Ad Mediation Settings */

/**
 * Call this method to retrieve a mediation settings object (if one is provided by the application) for this instance
 * of your ad.
 *
 * @param aClass The specific mediation settings class your custom event uses to configure itself for its ad network.
 */
- (id<MPMediationSettingsProtocol>)instanceMediationSettingsForClass:(Class)aClass;

/** @name Rewarded Video Ad Event Callbacks - Fetching Ads */

/**
 * Call this method immediately after an ad loads succesfully.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it successfully loads an ad.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)rewardedVideoDidLoadAdForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

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
- (void)rewardedVideoDidFailToLoadAdForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent error:(NSError *)error;

/**
 * Call this method if a previously loaded rewarded video should no longer be eligible for presentation.
 *
 * Some third-party networks will mark rewarded videos as expired (indicating they should not be
 * presented) *after* they have loaded.  You may use this method to inform the MoPub SDK that a
 * previously loaded rewarded video has expired and that a new rewarded video should be obtained.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 */
- (void)rewardedVideoDidExpireForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when the application has attempted to play a rewarded video and it cannot be played.
 *
 * A common usage of this delegate method is when the application tries to play an ad and an ad is not available for play.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param error The error describing why the video couldn't play.
 */
- (void)rewardedVideoDidFailToPlayForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent error:(NSError *)error;

/**
 * Call this method when an ad is about to appear.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is about to present the rewarded video.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)rewardedVideoWillAppearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when an ad has finished appearing.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is finished presenting the rewarded video.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 * **Note**: If it is not possible to know when the rewarded video *finished* appearing, you should call
 * this immediately after calling `-rewardedVideoWillAppearForCustomEvent:`.
 */
- (void)rewardedVideoDidAppearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when an ad is about to disappear.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is about to dismiss the rewarded video.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 */
- (void)rewardedVideoWillDisappearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when an ad has finished disappearing.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @warning **Important**: Your custom event subclass **must** call this method when it is finished with dismissing the rewarded video.
 * Failure to do so will disrupt the mediation waterfall and cause future ad requests to stall.
 *
 * **Note**: if it is not possible to know when the rewarded video *finished* dismissing, you should call
 * this immediately after calling `-rewardedVideoWillDisappearForCustomEvent:`.
 */
- (void)rewardedVideoDidDisappearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when the rewarded video ad will cause the user to leave the application.
 *
 * For example, the user may have tapped on the video which redirects the user to the App Store or Safari.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 */
- (void)rewardedVideoWillLeaveApplicationForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when the user taps on the rewarded video ad.
 *
 * This method is optional.  When automatic click and impression tracking are enabled (the default)
 * this method will track a click (the click is guaranteed to only be tracked once per ad).
 *
 * **Note**: some third-party networks provide a "will leave application" callback instead of/in
 * addition to a "user did click" callback. You should call this method in response to either of
 * those callbacks (since leaving the application is generally an indicator of a user tap).
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 */
- (void)rewardedVideoDidReceiveTapEventForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/**
 * Call this method when the user should be rewarded for watching the rewarded video.
 *
 * @param customEvent You should pass `self` to allow the MoPub SDK to associate this event with the correct
 * instance of your custom event.
 *
 * @param reward The reward object that contains the currency type as well as the amount that should be rewarded to
 * the user. If the concept of currency type doesn't exist for your ad network, set the reward's currency type as
 * kMPRewardedVideoRewardCurrencyTypeUnspecified.
 */
- (void)rewardedVideoShouldRewardUserForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent reward:(MPRewardedVideoReward *)reward;

/**
 * Call this method to get the customer ID associated with this custom event.
 *
 * @return The user's customer ID.
 */
- (NSString *)customerIdForRewardedVideoCustomEvent:(MPRewardedVideoCustomEvent *)customEvent;

/** @name Impression and Click Tracking */

/**
 * Call this method to track an impression.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPRewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks impressions.
 */
- (void)trackImpression;

/**
 * Call this method to track a click.
 *
 * @warning **Important**: You should **only** call this method if you have [opted out of automatic click and impression tracking]([MPRewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking]).
 * By default the MoPub SDK automatically tracks clicks.
 */
- (void)trackClick;

@end

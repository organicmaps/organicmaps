//
//  MPInterstitialAdController.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

@protocol MPInterstitialAdControllerDelegate;

/**
 * The `MPInterstitialAdController` class provides a full-screen advertisement that can be
 * displayed during natural transition points in your application.
 */

@interface MPInterstitialAdController : UIViewController

/** @name Obtaining an Interstitial Ad */

/**
 * Returns an interstitial ad object matching the given ad unit ID.
 *
 * The first time this method is called for a given ad unit ID, a new interstitial ad object is
 * created, stored in a shared pool, and returned. Subsequent calls for the same ad unit ID will
 * return that object, unless you have disposed of the object using
 * `removeSharedInterstitialAdController:`.
 *
 * There can only be one interstitial object for an ad unit ID at a given time.
 *
 * @param adUnitId A string representing a MoPub ad unit ID.
 */
+ (MPInterstitialAdController *)interstitialAdControllerForAdUnitId:(NSString *)adUnitId;

/** @name Setting and Getting the Delegate */

/**
 * The delegate (`MPInterstitialAdControllerDelegate`) of the interstitial ad object.
 */
@property (nonatomic, assign) id<MPInterstitialAdControllerDelegate> delegate;

/** @name Setting Request Parameters */

/**
 * The MoPub ad unit ID for this interstitial ad.
 *
 * Ad unit IDs are created on the MoPub website. An ad unit is a defined placement in your
 * application set aside for advertising. If no ad unit ID is set, the ad object will use a default
 * ID that only receives test ads.
 */
@property (nonatomic, copy) NSString *adUnitId;

/**
 * A string representing a set of keywords that should be passed to the MoPub ad server to receive
 * more relevant advertising.
 *
 * Keywords are typically used to target ad campaigns at specific user segments. They should be
 * formatted as comma-separated key-value pairs (e.g. "marital:single,age:24").
 *
 * On the MoPub website, keyword targeting options can be found under the "Advanced Targeting"
 * section when managing campaigns.
 */
@property (nonatomic, copy) NSString *keywords;

/**
 * A `CLLocation` object representing a user's location that should be passed to the MoPub ad server
 * to receive more relevant advertising.
 */
@property (nonatomic, copy) CLLocation *location;

/** @name Enabling Test Mode */

/**
 * A Boolean value that determines whether the interstitial ad object should request ads in test
 * mode.
 *
 * The default value is NO.
 * @warning **Important**: If you set this value to YES, make sure to reset it to NO before
 * submitting your application to the App Store.
 */
@property (nonatomic, assign, getter=isTesting) BOOL testing;

/** @name Loading an Interstitial Ad */

/**
 * Begins loading ad content for the interstitial.
 *
 * You can implement the `interstitialDidLoadAd:` and `interstitialDidFailToLoadAd:` methods of
 * `MPInterstitialAdControllerDelegate` if you would like to be notified as loading succeeds or
 * fails.
 */
- (void)loadAd;

/** @name Detecting Whether the Interstitial Ad Has Loaded */

/**
 * A Boolean value that represents whether the interstitial ad has loaded an advertisement and is
 * ready to be presented.
 *
 * After obtaining an interstitial ad object, you can use `loadAd` to tell the object to begin
 * loading ad content. Once the content has been loaded, the value of this property will be YES.
 *
 * The value of this property can be NO if the ad content has not finished loading, has already
 * been presented, or has expired. The expiration condition only applies for ads from certain
 * third-party ad networks. See `MPInterstitialAdControllerDelegate` for more details.
 */
@property (nonatomic, assign, readonly) BOOL ready;

/** @name Presenting an Interstitial Ad */

/**
 * Presents the interstitial ad modally from the specified view controller.
 *
 * This method will do nothing if the interstitial ad has not been loaded (i.e. the value of its
 * `ready` property is NO).
 *
 * `MPInterstitialAdControllerDelegate` provides optional methods that you may implement to stay
 * informed about when an interstitial takes over or relinquishes the screen.
 *
 * @param controller The view controller that should be used to present the interstitial ad.
 */
- (void)showFromViewController:(UIViewController *)controller;

/** @name Disposing of an Interstitial Ad */

/**
 * Removes the given interstitial object from the shared pool of interstitials available to your
 * application.
 *
 * This method removes the mapping from the interstitial's ad unit ID to the interstitial ad
 * object. In other words, you will receive a different ad object if you subsequently call
 * `interstitialAdControllerForAdUnitId:` for the same ad unit ID.
 *
 * @warning **Important**: This method is intended to be used for deallocating the interstitial
 * ad object when it is no longer needed. You should `nil` out any references you have to the
 * object after calling this method.
 *
 * @param controller The interstitial ad object that should be disposed.
 */
+ (void)removeSharedInterstitialAdController:(MPInterstitialAdController *)controller;

/*
 * Returns the shared pool of interstitial objects for your application.
 */
+ (NSMutableArray *)sharedInterstitialAdControllers;

#pragma mark - Deprecated

/** @name Handling Custom Event Methods (Deprecated) */

/**
 * Notifies MoPub that a custom event has successfully loaded an interstitial.
 *
 * @bug **Warning**: This method has been deprecated. You should instead implement interstitial
 * custom events using a subclass of `MPInterstitialCustomEvent`.
 */
- (void)customEventDidLoadAd __attribute__((deprecated));

/**
 * Notifies MoPub that a custom event has failed to load an interstitial.
 *
 * @bug **Warning**: This method has been deprecated. You should instead implement interstitial
 * custom events using a subclass of `MPInterstitialCustomEvent`.
 */
- (void)customEventDidFailToLoadAd __attribute__((deprecated));

/**
 * Notifies MoPub that a user has tapped on a custom event interstitial.
 *
 * @bug **Warning**: This method has been deprecated. You should instead implement interstitial
 * custom events using a subclass of `MPInterstitialCustomEvent`.
 */
- (void)customEventActionWillBegin __attribute__((deprecated));

@end

#pragma mark -

/**
 * The delegate of an `MPInterstitialAdController` object must adopt the
 * `MPInterstitialAdControllerDelegate` protocol.
 *
 * The optional methods of this protocol allow the delegate to be notified of interstitial state
 * changes, such as when an ad has loaded, when an ad has been presented or dismissed from the
 * screen, and when an ad has expired.
 */

@protocol MPInterstitialAdControllerDelegate <NSObject>

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
 * Interstitial ads from certain networks (such as iAd) may expire their content at any time,
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

/*
 * DEPRECATED: This callback notifies you to dismiss the interstitial, and allows you to implement
 * any pre-dismissal behavior (e.g. unpausing a game). This method is being deprecated as it is no
 * longer necessary to dismiss an interstitial manually (i.e. via calling
 * -dismissModalViewControllerAnimated:).
 *
 * Any pre-dismissal behavior should be implemented using -interstitialWillDisappear: or
 * -interstitialDidDisappear: instead.
 */
- (void)dismissInterstitial:(MPInterstitialAdController *)interstitial __attribute__((deprecated));

@end

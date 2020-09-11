//
//  MPInterstitialAdController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "MPInterstitialAdControllerDelegate.h"

/**
 * The `MPInterstitialAdController` class provides a full-screen advertisement that can be
 * displayed during natural transition points in your application.
 */

@interface MPInterstitialAdController : NSObject <MPMoPubAd>

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
@property (nonatomic, weak) id<MPInterstitialAdControllerDelegate> delegate;

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
 * A string representing a set of non-personally identifiable keywords that should be passed to the MoPub ad server to receive
 * more relevant advertising.

 * Note: If a user is in General Data Protection Regulation (GDPR) region and MoPub doesn't obtain consent from the user, "keywords" will still be sent to the server.
 *
 */
@property (nonatomic, copy) NSString *keywords;

/**
 * A string representing a set of personally identifiable keywords that should be passed to the MoPub ad server to receive
 * more relevant advertising.
 *
 * Keywords are typically used to target ad campaigns at specific user segments. They should be
 * formatted as comma-separated key-value pairs (e.g. "marital:single,age:24").
 *
 * On the MoPub website, keyword targeting options can be found under the "Advanced Targeting"
 * section when managing campaigns.
 *
 * Note: If a user is in General Data Protection Regulation (GDPR) region and MoPub doesn't obtain consent from the user, personally identifiable keywords will not be sent to the server.
 */
@property (nonatomic, copy) NSString *userDataKeywords;

/**
 * A `CLLocation` object representing a user's location that should be passed to the MoPub ad server
 * to receive more relevant advertising.
 * @deprecated This API is deprecated and will be removed in a future version.
 */
@property (nonatomic, copy) CLLocation *location __attribute__((deprecated("This API is deprecated and will be removed in a future version.")));

/**
 * An optional dictionary containing extra local data.
 */
@property (nonatomic, copy) NSDictionary *localExtras;

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
+ (NSMutableArray *)sharedInterstitialAdControllers DEPRECATED_MSG_ATTRIBUTE("This functionality will be removed in a future SDK release.");

@end

//
//  MPAdView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "MPConstants.h"
#import "MPAdViewDelegate.h"

typedef enum
{
    MPNativeAdOrientationAny,
    MPNativeAdOrientationPortrait,
    MPNativeAdOrientationLandscape
} MPNativeAdOrientation;

/**
 * The MPAdView class provides a view that can display banner advertisements.
 */
IB_DESIGNABLE
@interface MPAdView : UIView <MPMoPubAd>

/** @name Initializing a Banner Ad */

/**
 * Initializes an MPAdView with the given ad unit ID.
 *
 * @param adUnitId A string representing a MoPub ad unit ID.
 * @return A newly initialized ad view corresponding to the given ad unit ID and size.
 */
- (id)initWithAdUnitId:(NSString *)adUnitId;

/**
 * Initializes an MPAdView with the given ad unit ID and banner size.
 *
 * @param adUnitId A string representing a MoPub ad unit ID.
 * @param size The desired ad size. A list of standard ad sizes is available in MPConstants.h.
 * @return A newly initialized ad view corresponding to the given ad unit ID and size.
 */
- (id)initWithAdUnitId:(NSString *)adUnitId size:(CGSize)size __attribute__((deprecated("Use initWithAdUnitId: instead")));

/** @name Setting and Getting the Delegate */

/**
 * The delegate (`MPAdViewDelegate`) of the ad view.
 */
@property (nonatomic, weak) id<MPAdViewDelegate> delegate;

/** @name Setting Request Parameters */

/**
 * The MoPub ad unit ID for this ad view.
 *
 * Ad unit IDs are created on the MoPub website. An ad unit is a defined placement in your
 * application set aside for advertising. If no ad unit ID is set, the ad view will use a default
 * ID that only receives test ads.
 */
@property (nonatomic, copy) IBInspectable NSString *adUnitId;

/**
 * The maximum desired ad size. A list of standard ad sizes is available in MPConstants.h.
 */
@property (nonatomic, assign) IBInspectable CGSize maxAdSize;

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

* Note: If a user is in General Data Protection Regulation (GDPR) region and MoPub doesn't obtain consent from the user, "userDataKeywords" will not be sent to the server.
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

/** @name Loading a Banner Ad */

/**
 * Requests a new ad from the MoPub ad server with a maximum desired ad size equal to
 * the size of the current @c bounds of this view.
 *
 * If the ad view is already loading an ad, this call will be ignored. You may use `forceRefreshAd`
 * if you would like cancel any existing ad requests and force a new ad to load.
 */
- (void)loadAd;

/**
 * Requests a new ad from the MoPub ad server with the specified maximum desired ad size.
 *
 * If the ad view is already loading an ad, this call will be ignored. You may use `forceRefreshAd`
 * if you would like cancel any existing ad requests and force a new ad to load.
 *
 * @param size The maximum desired ad size to request. You may specify this value manually,
 * or use one of the presets found in @c MPConstants.h for the most common types of maximum ad sizes.
 * If using @c kMPPresetMaxAdSizeMatchFrame, the frame will be used as the maximum ad size.
 */
- (void)loadAdWithMaxAdSize:(CGSize)size;

/**
 * Cancels any existing ad requests and requests a new ad from the MoPub ad server
 * using the previously loaded maximum desired ad size.
 */
- (void)forceRefreshAd;

/** @name Handling Orientation Changes */

/**
 * Informs the ad view that the device orientation has changed.
 *
 * Banners from some third-party ad networks have orientation-specific behavior. You should call
 * this method when your application's orientation changes if you want mediated ads to acknowledge
 * their new orientation.
 *
 * If your application layout needs to change based on the size of the mediated ad, you may want to
 * check the value of `adContentViewSize` after calling this method, in case the orientation change
 * causes the mediated ad to resize.
 *
 * @param newOrientation The new interface orientation (after orientation changes have occurred).
 */
- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation;

/**
 * Forces third-party native ad networks to only use ads sized for the specified orientation.
 *
 * Banners from some third-party ad networks have orientation-specific behaviors and/or sizes.
 * You may use this method to lock ads to a certain orientation. For instance,
 * if you call this with MPInterfaceOrientationPortrait, native networks will never
 * return ads sized for the landscape orientation.
 *
 * @param orientation An MPNativeAdOrientation enum value.
 *
 * <pre><code>typedef enum {
 *          MPNativeAdOrientationAny,
 *          MPNativeAdOrientationPortrait,
 *          MPNativeAdOrientationLandscape
 *      } MPNativeAdOrientation;
 * </code></pre>
 *
 * @see unlockNativeAdsOrientation
 * @see allowedNativeAdsOrientation
 */
- (void)lockNativeAdsToOrientation:(MPNativeAdOrientation)orientation;

/**
 * Allows third-party native ad networks to use ads sized for any orientation.
 *
 * You do not need to call this method unless you have previously called
 * `lockNativeAdsToOrientation:`.
 *
 * @see lockNativeAdsToOrientation:
 * @see allowedNativeAdsOrientation
 */
- (void)unlockNativeAdsOrientation;

/**
 * Returns the banner orientations that third-party ad networks are allowed to use.
 *
 * @return An enum value representing an allowed set of orientations.
 *
 * @see lockNativeAdsToOrientation:
 * @see unlockNativeAdsOrientation
 */
- (MPNativeAdOrientation)allowedNativeAdsOrientation;

/** @name Obtaining the Size of the Current Ad */

/**
 * Returns the size of the current ad being displayed in the ad view.
 *
 * Ad sizes may vary between different ad networks. This method returns the actual size of the
 * underlying mediated ad. This size may be different from the original, initialized size of the
 * ad view. You may use this size to determine to adjust the size or positioning of the ad view
 * to avoid clipping or border issues.
 *
 * @returns The size of the underlying mediated ad.
 */
- (CGSize)adContentViewSize;

/** @name Managing the Automatic Refreshing of Ads */

/**
 * Stops the ad view from periodically loading new advertisements.
 *
 * By default, an ad view is allowed to automatically load new advertisements if a refresh interval
 * has been configured on the MoPub website. This method prevents new ads from automatically loading,
 * even if a refresh interval has been specified.
 *
 * As a best practice, you should call this method whenever the ad view will be hidden from the user
 * for any period of time, in order to avoid unnecessary ad requests. You can then call
 * `startAutomaticallyRefreshingContents` to re-enable the refresh behavior when the ad view becomes
 * visible.
 *
 * @see startAutomaticallyRefreshingContents
 */
- (void)stopAutomaticallyRefreshingContents;

/**
 * Causes the ad view to periodically load new advertisements in accordance with user-defined
 * refresh settings on the MoPub website.
 *
 * Calling this method is only necessary if you have previously stopped the ad view's refresh
 * behavior using `stopAutomaticallyRefreshingContents`. By default, an ad view is allowed to
 * automatically load new advertisements if a refresh interval has been configured on the MoPub
 * website. This method has no effect if a refresh interval has not been set.
 *
 * @see stopAutomaticallyRefreshingContents
 */
- (void)startAutomaticallyRefreshingContents;

@end

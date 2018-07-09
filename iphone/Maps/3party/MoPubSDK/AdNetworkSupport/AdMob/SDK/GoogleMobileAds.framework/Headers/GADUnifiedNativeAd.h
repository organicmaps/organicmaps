//
//  GADUnifiedNativeAd.h
//  Google Mobile Ads SDK
//
//  Copyright 2017 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <GoogleMobileAds/GADAdChoicesView.h>
#import <GoogleMobileAds/GADAdLoaderDelegate.h>
#import <GoogleMobileAds/GADMediaView.h>
#import <GoogleMobileAds/GADNativeAdImage.h>
#import <GoogleMobileAds/GADVideoController.h>
#import <GoogleMobileAds/GADUnifiedNativeAdAssetIdentifiers.h>
#import <GoogleMobileAds/GADUnifiedNativeAdDelegate.h>
#import <GoogleMobileAds/GoogleMobileAdsDefines.h>

GAD_ASSUME_NONNULL_BEGIN

/// Unified native ad. To request this ad type, pass kGADAdLoaderAdTypeUnifiedNative
/// (see GADAdLoaderAdTypes.h) to the |adTypes| parameter in GADAdLoader's initializer method. If
/// you request this ad type, your delegate must conform to the GADUnifiedNativeAdLoaderDelegate
/// protocol.
@interface GADUnifiedNativeAd : NSObject

#pragma mark - Must be displayed if available

/// Headline
@property(nonatomic, readonly, copy, nullable) NSString *headline;

#pragma mark - Recommended to display

/// Text that encourages user to take some action with the ad. For example "Install".
@property(nonatomic, readonly, copy, nullable) NSString *callToAction;
/// Icon image.
@property(nonatomic, readonly, strong, nullable) GADNativeAdImage *icon;
/// Description.
@property(nonatomic, readonly, copy, nullable) NSString *body;
/// Array of GADNativeAdImage objects.
@property(nonatomic, readonly, strong, nullable) NSArray<GADNativeAdImage *> *images;
/// App store rating (0 to 5).
@property(nonatomic, readonly, copy, nullable) NSDecimalNumber *starRating;
/// The app store name. For example, "App Store".
@property(nonatomic, readonly, copy, nullable) NSString *store;
/// String representation of the app's price.
@property(nonatomic, readonly, copy, nullable) NSString *price;
/// Identifies the advertiser. For example, the advertiser’s name or visible URL.
@property(nonatomic, readonly, copy, nullable) NSString *advertiser;
/// Video controller for controlling video playback in GADUnifiedNativeAdView's mediaView.
@property(nonatomic, strong, readonly, nullable) GADVideoController *videoController;

/// Optional delegate to receive state change notifications.
@property(nonatomic, weak, nullable) id<GADUnifiedNativeAdDelegate> delegate;

/// Root view controller for handling ad actions.
@property(nonatomic, weak, nullable) UIViewController *rootViewController;

/// Dictionary of assets which aren't processed by the receiver.
@property(nonatomic, readonly, copy, nullable) NSDictionary<NSString *, id> *extraAssets;

/// The ad network class name that fetched the current ad. For both standard and mediated Google
/// AdMob ads, this method returns @"GADMAdapterGoogleAdMobAds". For ads fetched via mediation
/// custom events, this method returns @"GADMAdapterCustomEvents".
@property(nonatomic, readonly, copy, nullable) NSString *adNetworkClassName;

/// Registers ad view, clickable asset views, and nonclickable asset views with this native ad.
/// Media view shouldn't be registered as clickable.
/// @param clickableAssetViews Dictionary of asset views that are clickable, keyed by asset IDs.
/// @param nonclickableAssetViews Dictionary of asset views that are not clickable, keyed by asset
///        IDs.
- (void)registerAdView:(UIView *)adView
       clickableAssetViews:
           (NSDictionary<GADUnifiedNativeAssetIdentifier, UIView *> *)clickableAssetViews
    nonclickableAssetViews:
        (NSDictionary<GADUnifiedNativeAssetIdentifier, UIView *> *)nonclickableAssetViews;

/// Unregisters ad view from this native ad. The corresponding asset views will also be
/// unregistered.
- (void)unregisterAdView;

@end

#pragma mark - Protocol and constants

/// The delegate of a GADAdLoader object implements this protocol to receive GADUnifiedNativeAd ads.
@protocol GADUnifiedNativeAdLoaderDelegate<GADAdLoaderDelegate>
/// Called when a unified native ad is received.
- (void)adLoader:(GADAdLoader *)adLoader didReceiveUnifiedNativeAd:(GADUnifiedNativeAd *)nativeAd;
@end

#pragma mark - Unified Native Ad View

/// Base class for native ad views. Your native ad view must be a subclass of this class and must
/// call superclass methods for all overridden methods.
@interface GADUnifiedNativeAdView : UIView

/// This property must point to the unified native ad object rendered by this ad view.
@property(nonatomic, strong, nullable) GADUnifiedNativeAd *nativeAd;

/// Weak reference to your ad view's headline asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *headlineView;
/// Weak reference to your ad view's call to action asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *callToActionView;
/// Weak reference to your ad view's icon asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *iconView;
/// Weak reference to your ad view's body asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *bodyView;
/// Weak reference to your ad view's store asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *storeView;
/// Weak reference to your ad view's price asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *priceView;
/// Weak reference to your ad view's image asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *imageView;
/// Weak reference to your ad view's star rating asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *starRatingView;
/// Weak reference to your ad view's advertiser asset view.
@property(nonatomic, weak, nullable) IBOutlet UIView *advertiserView;
/// Weak reference to your ad view's media asset view.
@property(nonatomic, weak, nullable) IBOutlet GADMediaView *mediaView;
/// Weak reference to your ad view's AdChoices view. Must set adChoicesView before setting
/// nativeAd, otherwise AdChoices will be rendered in the publisher's preferredAdChoicesPosition as
/// defined in GADNativeAdViewAdOptions.
@property(nonatomic, weak, nullable) IBOutlet GADAdChoicesView *adChoicesView;

@end

GAD_ASSUME_NONNULL_END

//
//  MPNativeAdAdapter.h
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol MPNativeAdAdapter;

/**
 * Classes that conform to the `MPNativeAdAdapter` protocol can have an
 * `MPNativeAdAdapterDelegate` delegate object. You use this delegate to communicate
 * native ad events (such as impressions and clicks occurring) back to the MoPub SDK.
 */
@protocol MPNativeAdAdapterDelegate <NSObject>

@required

/**
 * Asks the delegate for a view controller to use for presenting modal content, such as the in-app
 * browser that can appear when an ad is tapped.
 *
 * @return A view controller that should be used for presenting modal content.
 */
- (UIViewController *)viewControllerForPresentingModalView;

/**
 * You should call this method when your adapter's modal is about to be presented.
 *
 * @param adapter The adapter that will present the modal.
 */
- (void)nativeAdWillPresentModalForAdapter:(id<MPNativeAdAdapter>)adapter;

/**
 * You should call this method when your adapter's modal has been dismissed.
 *
 * @param adapter The adapter that dismissed the modal.
 */
- (void)nativeAdDidDismissModalForAdapter:(id<MPNativeAdAdapter>)adapter;

/**
 * You should call this method when your the user will leave the application due to interaction with the ad.
 *
 * @param adapter The adapter that represents the ad that caused the user to leave the application.
 */
- (void)nativeAdWillLeaveApplicationFromAdapter:(id<MPNativeAdAdapter>)adapter;

@optional

/**
 * This method is called before the backing native ad logs an impression.
 *
 * @param adAdapter You should pass `self` to allow the MoPub SDK to associate this event with the
 * correct instance of your ad adapter.
 */
- (void)nativeAdWillLogImpression:(id<MPNativeAdAdapter>)adAdapter;

/**
 * This method is called when the user interacts with the ad.
 *
 * @param adAdapter You should pass `self` to allow the MoPub SDK to associate this event with the
 * correct instance of your ad adapter.
 */
- (void)nativeAdDidClick:(id<MPNativeAdAdapter>)adAdapter;

@end

/**
 * The `MPNativeAdAdapter` protocol allows the MoPub SDK to interact with native ad objects obtained
 * from third-party ad networks. An object that adopts this protocol acts as a wrapper for a native
 * ad object, translating its proprietary interface into a common one that the MoPub SDK can
 * understand.
 *
 * An object that adopts this protocol must implement the `properties` property to specify a
 * dictionary of assets, such as title and text, that should be rendered as part of a native ad.
 * When possible, you should place values in the returned dictionary such that they correspond to
 * the pre-defined keys in the MPNativeAdConstants header file.
 *
 * An adopting object must additionally implement -displayContentForURL:rootViewController:completion:
 * to supply the behavior that should occur when the user interacts with the ad.
 *
 * Optional methods of the protocol allow the adopting object to define when and how impressions
 * and interactions should be tracked.
 */
@protocol MPNativeAdAdapter <NSObject>

@required

/** @name Ad Resources */

/**
 * Provides a dictionary of all publicly accessible assets (such as title and text) for the
 * native ad.
 *
 * When possible, you should place values in the returned dictionary such that they correspond to
 * the pre-defined keys in the MPNativeAdConstants header file.
 */
@property (nonatomic, readonly) NSDictionary *properties;

/**
 * The default click-through URL for the ad.
 *
 * This may safely be set to nil if your network doesn't expose this value (for example, it may only
 * provide a method to handle a click, lacking another for retrieving the URL itself).
 */
@property (nonatomic, readonly) NSURL *defaultActionURL;

/** @name Handling Ad Interactions */

@optional

/**
 * Tells the object to open the specified URL using an appropriate mechanism.
 *
 * @param URL The URL to be opened.
 * @param controller The view controller that should be used to present the modal view controller.
 *
 * Your implementation of this method should either forward the request to the underlying
 * third-party ad object (if it has built-in support for handling ad interactions), or open an
 * in-application modal web browser or a modal App Store controller.
 */
- (void)displayContentForURL:(NSURL *)URL rootViewController:(UIViewController *)controller;

/**
 * Determines whether MPNativeAd should track clicks
 *
 * If not implemented, this will be assumed to return NO, and MPNativeAd will track clicks.
 * If this returns YES, then MPNativeAd will defer to the MPNativeAdAdapterDelegate callbacks to
 * track clicks.
 */
- (BOOL)enableThirdPartyClickTracking;

/**
 * Tracks a click for this ad.
 *
 * To avoid reporting discrepancies, you should only implement this method if the third-party ad
 * network requires clicks to be reported manually.
 */
- (void)trackClick;

/**
 * The `MPNativeAdAdapterDelegate` to send messages to as events occur.
 *
 * The `delegate` object defines several methods that you should call in order to inform MoPub
 * of interactions with the ad. This delegate needs to be implemented if third party impression and/or
 * click tracking is enabled.
 */
@property (nonatomic, weak) id<MPNativeAdAdapterDelegate> delegate;

/** @name Responding to an Ad Being Attached to a View */

/**
 * This method will be called when your ad's content is about to be loaded into a view.
 *
 * @param view A view that will contain the ad content.
 *
 * You should implement this method if the underlying third-party ad object needs to be informed
 * of this event.
 */
- (void)willAttachToView:(UIView *)view;

/**
 * This method will be called if your implementation provides a DAA icon through the properties dictionary
 * and the user has tapped the icon.
 */
- (void)displayContentForDAAIconTap;

/**
 * Return your ad's privacy information icon view.
 *
 * You should implement this method if your ad supplies its own view for its privacy information icon.
 */
- (UIView *)privacyInformationIconView;

/**
 * Return your ad's main media view.
 *
 * You should implement this method if your ad supplies its own view for the main media view which is typically
 * an image or video. If you implement this method, the SDK will not make any other attempts at retrieving
 * the main media asset.
 */
- (UIView *)mainMediaView;

@end

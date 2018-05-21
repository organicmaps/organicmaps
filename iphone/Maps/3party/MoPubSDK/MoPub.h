//
//  MoPub.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPConstants.h"

#import "MOPUBDisplayAgentType.h"
#import "MPAdConversionTracker.h"
#import "MPAdvancedBidder.h"
#import "MPAdView.h"
#import "MPBannerCustomEvent.h"
#import "MPBannerCustomEventDelegate.h"
#import "MPBool.h"
#import "MPConsentChangedNotification.h"
#import "MPConsentError.h"
#import "MPConsentStatus.h"
#import "MPGlobal.h"
#import "MPIdentityProvider.h"
#import "MPInterstitialAdController.h"
#import "MPInterstitialCustomEvent.h"
#import "MPInterstitialCustomEventDelegate.h"
#import "MPLogging.h"
#import "MPLogLevel.h"
#import "MPLogProvider.h"
#import "MPMediationSdkInitializable.h"
#import "MPMediationSettingsProtocol.h"
#import "MPMoPubConfiguration.h"
#import "MPRewardedVideo.h"
#import "MPRewardedVideoReward.h"
#import "MPRewardedVideoCustomEvent.h"
#import "MPRewardedVideoCustomEvent+Caching.h"
#import "MPRewardedVideoError.h"
#import "MPViewabilityAdapter.h"
#import "MPViewabilityOption.h"

#if MP_HAS_NATIVE_PACKAGE
#import "MPNativeAd.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeCustomEvent.h"
#import "MPNativeCustomEventDelegate.h"
#import "MPNativeAdError.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdRequest.h"
#import "MPNativeAdRequestTargeting.h"
#import "MPCollectionViewAdPlacer.h"
#import "MPTableViewAdPlacer.h"
#import "MPClientAdPositioning.h"
#import "MPServerAdPositioning.h"
#import "MPNativeAdDelegate.h"
#import "MPStaticNativeAdRendererSettings.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRendererSettings.h"
#import "MPNativeAdRenderer.h"
#import "MPStaticNativeAdRenderer.h"
#import "MOPUBNativeVideoAdRendererSettings.h"
#import "MOPUBNativeVideoAdRenderer.h"
#import "MPNativeAdRenderingImageLoader.h"
#import "MPStreamAdPlacer.h"
#endif

// Import these frameworks for module support.
#import <AdSupport/AdSupport.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <MessageUI/MessageUI.h>
#import <QuartzCore/QuartzCore.h>
#import <StoreKit/StoreKit.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <UIKit/UIKit.h>

#define MoPubKit [MoPub sharedInstance]

NS_ASSUME_NONNULL_BEGIN

@interface MoPub : NSObject

/**
 * Returns the MoPub singleton object.
 *
 * @return The MoPub singleton object.
 */
+ (MoPub * _Nonnull)sharedInstance;

/**
 * A Boolean value indicating whether the MoPub SDK should use Core Location APIs to automatically
 * derive targeting information for location-based ads.
 *
 * When set to NO, the SDK will not attempt to determine device location. When set to YES, the
 * SDK will periodically try to listen for location updates in order to request location-based ads.
 * This only occurs if location services are enabled and the user has already authorized the use
 * of location services for the application. The default value is YES.
 *
 * If a user is in General Data Protection Regulation (GDPR) region and
 * MoPub doesn't obtain consent from the user for using his/her personal data,
 * locationUpdatesEnabled will always be set to NO.
 *
 * @return A Boolean value indicating whether the SDK should listen for location updates.
 */
@property (nonatomic, assign) BOOL locationUpdatesEnabled;

/**
 * A Boolean value indicating whether the MoPub SDK should create a MoPub ID that can be used
 * for frequency capping when Limit ad tracking is on & the IDFA we get is
 * 00000000-0000-0000-0000-000000000000.
 *
 * When set to NO, the SDK will not create a MoPub ID in the above case. When set to YES, the
 * SDK will generate a MoPub ID. The default value is YES.
 *
 */
@property (nonatomic) BOOL frequencyCappingIdUsageEnabled;

/**
 * Forces the usage of WKWebView (if able).
 */
@property (nonatomic, assign) BOOL forceWKWebView;

/**
 * SDK log level. The default value is `MPLogLevelInfo`.
 */
@property (nonatomic, assign) MPLogLevel logLevel;

/**
 * A boolean value indicating whether advanced bidding is enabled. This boolean defaults to `YES`.
 * To disable advanced bidding, set this value to `NO`.
 */
@property (nonatomic, assign) BOOL enableAdvancedBidding;

/**
 * Initializes the MoPub SDK asynchronously on a background thread.
 * @remark This should be called from the app's `application:didFinishLaunchingWithOptions:` method.
 * @param configuration Required SDK configuration options.
 * @param completionBlock Optional completion block that will be called when initialization has completed.
 */
- (void)initializeSdkWithConfiguration:(MPMoPubConfiguration * _Nonnull)configuration
                            completion:(void(^_Nullable)(void))completionBlock;

/**
 * A boolean value indicating if the SDK has been initialized. This property's value is @c YES if
 * @c initializeSdkWithConfiguration:completion: has been called and completed; the value is @c NO otherwise.
 */
@property (nonatomic, readonly) BOOL isSdkInitialized;

/**
 * Retrieves the global mediation settings for a given class type.
 *
 * @param aClass The type of mediation settings object you want to receive from the collection.
 */
- (id<MPMediationSettingsProtocol> _Nullable)globalMediationSettingsForClass:(Class)aClass;

- (NSString * _Nonnull)version;
- (NSString * _Nonnull)bundleIdentifier;

/**
 * Default is MOPUBDisplayAgentTypeInApp = 0.
 *
 * If displayType is set to MOPUBDisplayAgentTypeNativeSafari = 1, http/https clickthrough URLs are opened in native
 * safari browser.
 * If displayType is set to MOPUBDisplayAgentTypeSafariViewController = 2, http/https clickthrough URLs are opened in
 * SafariViewController.
 *
 */
- (void)setClickthroughDisplayAgentType:(MOPUBDisplayAgentType)displayAgentType;

/**
 * Disables viewability measurement via the specified vendor(s) for the rest of the app session. A given vendor cannot
 * be re-enabled after being disabled.
 *
 * @param vendors The viewability vendor(s) to be disabled. This is a bitmask value; ORing vendors together is okay.
 */
- (void)disableViewability:(MPViewabilityOption)vendors;

@end

@interface MoPub (Mediation)
/**
 * Retrieves all currently cached mediated networks.
 * @return A list of all cached networks or @c nil.
 */
- (NSArray<Class<MPMediationSdkInitializable>> * _Nullable)allCachedNetworks;

/**
 * Clears all currently cached mediated networks.
 */
- (void)clearCachedNetworks;

@end

@interface MoPub (Consent)

/**
 * Querying Consent State
 */

/**
 * Flag indicating that personally identifiable information can be collected.
 */
@property (nonatomic, readonly) BOOL canCollectPersonalInfo;

/**
 * Gives the current consent status of this user.
 * Note: NSNotification with name @c kMPConsentChangedNotification can be listened for to be informed of changes
 * in the @c currentConsentStatus value. Please see MPConsentChangedNotification.h for more information on this
 * notification.
 */
@property (nonatomic, readonly) MPConsentStatus currentConsentStatus;

/**
 * Flag indicating that GDPR is applicable to the user.
 */
@property (nonatomic, readonly) MPBool isGDPRApplicable;

/**
 * Consent Acquisition
 */

/**
 * `YES` if a consent dialog is presently loaded and ready to be shown; `NO` otherwise
 */
@property (nonatomic, readonly) BOOL isConsentDialogReady;

/**
 * Attempts to load a consent dialog. `completion` is called when either the consent dialog has finished loading
 * or has failed to load. If there was an error, the `error` parameter will be non-nil.
 */
- (void)loadConsentDialogWithCompletion:(void (^ _Nullable)(NSError * _Nullable error))completion;

/**
 * If a consent dialog is currently loaded, this method will present it modally on top of `viewController`. If no
 * consent dialog is loaded, this method will do nothing. Completion is called upon successful presentation of the
 * consent dialog; otherwise it is not called.
 */
- (void)showConsentDialogFromViewController:(UIViewController *)viewController completion:(void (^ _Nullable)(void))completion;

/**
 * Flag indicating that consent needs to be acquired (or reacquired) by the user, and that the consent dialog may need
 * to be shown. (Note: This flag can be used for publishers that require use of MoPub's consent dialog, as well as
 * publishers that specify their own consent interface)
 */
@property (nonatomic, readonly) BOOL shouldShowConsentDialog;

/**
 * Custom Consent Interface
 * Note: publishers must have explicit permission from MoPub to use their own consent interface.
 */

/**
 URL to the MoPub privacy policy in the device's preferred language. If the device's
 preferred language could not be determined, English will be used.
 @returns The privacy policy URL for the desired language if successful; @c nil if
 there is no current vendor list.
 */
- (NSURL * _Nullable)currentConsentPrivacyPolicyUrl;

/**
 URL to the MoPub privacy policy in the language of choice.
 @param isoLanguageCode ISO 630-1 language code
 @returns The privacy policy URL for the desired language if successful; @c nil if the
 language code is invalid or if there is no current vendor list.
 */
- (NSURL * _Nullable)currentConsentPrivacyPolicyUrlWithISOLanguageCode:(NSString * _Nonnull)isoLanguageCode;

/**
 Current vendor list URL in the device's preferred language. If the device's
 preferred language could not be determined, English will be used.
 @returns The vendor list URL for the desired language if successful; @c nil if
 there is no current vendor list.
 */
- (NSURL * _Nullable)currentConsentVendorListUrl;

/**
 Current vendor list URL in the language of choice.
 @param isoLanguageCode ISO 630-1 language code
 @returns The vendor list URL for the desired language if successful; @c nil if the
 language code is invalid or if there is no current vendor list.
 */
- (NSURL * _Nullable)currentConsentVendorListUrlWithISOLanguageCode:(NSString * _Nonnull)isoLanguageCode;

/**
 * Grants consent on behalf of the current user. If you do not have permission from MoPub to use a custom consent
 * interface, this method will always fail to grant consent.
 */
- (void)grantConsent;

/**
 * Revokes consent on behalf of the current user.
 */
- (void)revokeConsent;

/**
 * Current IAB format vendor list.
 */
@property (nonatomic, copy, readonly, nullable) NSString * currentConsentIabVendorListFormat;

/**
 * Current version of MoPub’s privacy policy.
 */
@property (nonatomic, copy, readonly, nullable) NSString * currentConsentPrivacyPolicyVersion;

/**
 * Current version of the vendor list.
 */
@property (nonatomic, copy, readonly, nullable) NSString * currentConsentVendorListVersion;

/**
 * IAB vendor list that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * previouslyConsentedIabVendorListFormat;

/**
 * MoPub privacy policy version that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * previouslyConsentedPrivacyPolicyVersion;

/**
 * Vendor list version that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * previouslyConsentedVendorListVersion;

@end

NS_ASSUME_NONNULL_END

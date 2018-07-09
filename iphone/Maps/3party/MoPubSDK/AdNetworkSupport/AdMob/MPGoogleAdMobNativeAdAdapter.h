#if __has_include(<MoPub / MoPub.h>)
#import <MoPub/MoPub.h>
#else
#import "MPNativeAdAdapter.h"
#endif

#import <GoogleMobileAds/GoogleMobileAds.h>

/// This class implements the `MPNativeAdAdapter` and `GADNativeAdDelegate` protocols, that allow
/// the MoPub SDK to interact with native ad objects obtained from Google Mobile Ads SDK.
@interface MPGoogleAdMobNativeAdAdapter : NSObject<MPNativeAdAdapter, GADNativeAdDelegate>

/// MoPub native ad adapter delegate instance.
@property(nonatomic, weak) id<MPNativeAdAdapterDelegate> delegate;

/// Google Mobile Ads native app install ad instance.
@property(nonatomic, strong) GADNativeAppInstallAd *adMobNativeAppInstallAd;

/// Google Mobile Ads native content ad instance.
@property(nonatomic, strong) GADNativeContentAd *adMobNativeContentAd;

/// Google Mobile Ads container view to hold the AdChoices icon.
@property(nonatomic, strong) GADAdChoicesView *adChoicesView;

/// Returns an MPGoogleAdMobNativeAdAdapter with GADNativeContentAd.
- (instancetype)initWithAdMobNativeContentAd:(GADNativeContentAd *)adMobNativeContentAd;

/// Returns an MPGoogleAdMobNativeAdAdapter with GADNativeAppInstallAd.
- (instancetype)initWithAdMobNativeAppInstallAd:(GADNativeAppInstallAd *)adMobNativeAppInstallAd;

@end

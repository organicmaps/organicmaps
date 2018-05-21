//
//  GADUnifiedNativeAdUnconfirmedClickDelegate.h
//  Google Mobile Ads SDK
//
//  Copyright 2017 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <GoogleMobileAds/GADUnifiedNativeAdAssetIdentifiers.h>
#import <GoogleMobileAds/GoogleMobileAdsDefines.h>

@class GADUnifiedNativeAd;

GAD_ASSUME_NONNULL_BEGIN

@protocol GADUnifiedNativeAdUnconfirmedClickDelegate<NSObject>

/// Tells the delegate that native ad receives an unconfirmed click on view with asset ID. You
/// should update user interface and ask user to confirm the click once this message is received.
/// Use the -registerClickConfirmingView: method in GADNUnifiedNativeAd+ConfirmationClick.h to
/// register a view that will confirm the click. Only called for Google ads and is not supported for
/// mediated ads.
- (void)nativeAd:(GADUnifiedNativeAd *)nativeAd
    didReceiveUnconfirmedClickOnAssetID:(GADUnifiedNativeAssetIdentifier)assetID;

/// Tells the delegate that the unconfirmed click is cancelled. You should revert the user interface
/// change once this message is received. Only called for Google ads and is not supported for
/// mediated ads.
- (void)nativeAdDidCancelUnconfirmedClick:(GADUnifiedNativeAd *)nativeAd;

@end

GAD_ASSUME_NONNULL_END

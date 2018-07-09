//
//  GADUnifiedNativeAd+ConfirmationClick.h
//  Google Mobile Ads SDK
//
//  Copyright 2017 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <GoogleMobileAds/GADUnifiedNativeAdUnconfirmedClickDelegate.h>
#import <GoogleMobileAds/GADUnifiedNativeAd.h>
#import <GoogleMobileAds/GoogleMobileAdsDefines.h>

GAD_ASSUME_NONNULL_BEGIN

@interface GADUnifiedNativeAd (ConfirmationClick)

/// Unconfirmed click delegate.
@property(nonatomic, weak, nullable)
    id<GADUnifiedNativeAdUnconfirmedClickDelegate> unconfirmedClickDelegate;

/// Registers a view that will confirm the click.
- (void)registerClickConfirmingView:(nullable UIView *)view;

/// Cancels the unconfirmed click. Called when user fails to confirm the click. When this method is
/// called, SDK stops tracking click on the registered click confirming view and invokes the
/// -nativeAdDidCancelUnconfirmedClick: delegate method. If there's no ongoing unconfirmed click,
/// this method is no-op.
- (void)cancelUnconfirmedClick;

@end

GAD_ASSUME_NONNULL_END

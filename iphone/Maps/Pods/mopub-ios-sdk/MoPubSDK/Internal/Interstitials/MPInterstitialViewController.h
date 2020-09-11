//
//  MPInterstitialViewController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPExtendedHitBoxButton.h"
#import "MPGlobal.h"

@class CLLocation;

/**
 The purpose of this @c MPInterstitialViewController protocol is to define the common interface
 between interstitial view controllers without forcing them to subclass @c MPInterstitialViewController.
 */
@protocol MPInterstitialViewController <NSObject>
@end

@protocol MPInterstitialViewControllerDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPInterstitialViewController : UIViewController <MPInterstitialViewController>

@property (nonatomic, assign) MPInterstitialCloseButtonStyle closeButtonStyle;
@property (nonatomic, assign) MPInterstitialOrientationType orientationType;
@property (nonatomic, strong) MPExtendedHitBoxButton *closeButton;
@property (nonatomic, weak) id<MPInterstitialViewControllerDelegate> delegate;

- (void)presentInterstitialFromViewController:(UIViewController *)controller complete:(void(^)(NSError *))complete;
- (void)dismissInterstitialAnimated:(BOOL)animated;
- (BOOL)shouldDisplayCloseButton;
- (void)willPresentInterstitial;
- (void)didPresentInterstitial;
- (void)willDismissInterstitial;
- (void)didDismissInterstitial;
- (void)layoutCloseButton;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPInterstitialViewControllerAppearanceDelegate <NSObject>

- (void)interstitialWillAppear:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialDidAppear:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialWillDisappear:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialDidDisappear:(id<MPInterstitialViewController>)interstitial;

@end

@protocol MPInterstitialViewControllerDelegate <MPInterstitialViewControllerAppearanceDelegate>

- (void)interstitialDidLoadAd:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialDidFailToLoadAd:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialDidReceiveTapEvent:(id<MPInterstitialViewController>)interstitial;
- (void)interstitialWillLeaveApplication:(id<MPInterstitialViewController>)interstitial;

@optional

- (void)interstitialRewardedVideoEnded;

@end

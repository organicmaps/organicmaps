//
//  MPInterstitialViewController.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPGlobal.h"

@class CLLocation;

@protocol MPInterstitialViewControllerDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPInterstitialViewController : UIViewController

@property (nonatomic, assign) MPInterstitialCloseButtonStyle closeButtonStyle;
@property (nonatomic, assign) MPInterstitialOrientationType orientationType;
@property (nonatomic, retain) UIButton *closeButton;
@property (nonatomic, assign) id<MPInterstitialViewControllerDelegate> delegate;

- (void)presentInterstitialFromViewController:(UIViewController *)controller;
- (void)dismissInterstitialAnimated:(BOOL)animated;
- (BOOL)shouldDisplayCloseButton;
- (void)willPresentInterstitial;
- (void)didPresentInterstitial;
- (void)willDismissInterstitial;
- (void)didDismissInterstitial;
- (void)layoutCloseButton;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPInterstitialViewControllerDelegate <NSObject>

- (NSString *)adUnitId;
- (CLLocation *)location;
- (void)interstitialDidLoadAd:(MPInterstitialViewController *)interstitial;
- (void)interstitialDidFailToLoadAd:(MPInterstitialViewController *)interstitial;
- (void)interstitialWillAppear:(MPInterstitialViewController *)interstitial;
- (void)interstitialDidAppear:(MPInterstitialViewController *)interstitial;
- (void)interstitialWillDisappear:(MPInterstitialViewController *)interstitial;
- (void)interstitialDidDisappear:(MPInterstitialViewController *)interstitial;
- (void)interstitialWillLeaveApplication:(MPInterstitialViewController *)interstitial;

@end

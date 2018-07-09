//
//  MPBaseInterstitialAdapter.h
//  MoPub
//
//  Created by Nafis Jamal on 4/27/11.
//  Copyright 2011 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class MPAdConfiguration, CLLocation;

@protocol MPInterstitialAdapterDelegate;

@interface MPBaseInterstitialAdapter : NSObject

@property (nonatomic, weak) id<MPInterstitialAdapterDelegate> delegate;

/*
 * Creates an adapter with a reference to an MPInterstitialAdManager.
 */
- (id)initWithDelegate:(id<MPInterstitialAdapterDelegate>)delegate;

/*
 * Sets the adapter's delegate to nil.
 */
- (void)unregisterDelegate;

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration;
- (void)_getAdWithConfiguration:(MPAdConfiguration *)configuration;

- (void)didStopLoading;

/*
 * Presents the interstitial from the specified view controller.
 */
- (void)showInterstitialFromViewController:(UIViewController *)controller;

@end

@interface MPBaseInterstitialAdapter (ProtectedMethods)

- (void)trackImpression;
- (void)trackClick;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@class MPInterstitialAdController;

@protocol MPInterstitialAdapterDelegate

- (MPInterstitialAdController *)interstitialAdController;
- (id)interstitialDelegate;
- (CLLocation *)location;

- (void)adapterDidFinishLoadingAd:(MPBaseInterstitialAdapter *)adapter;
- (void)adapter:(MPBaseInterstitialAdapter *)adapter didFailToLoadAdWithError:(NSError *)error;
- (void)interstitialWillAppearForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialDidAppearForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialWillDisappearForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialDidDisappearForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialDidExpireForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialDidReceiveTapEventForAdapter:(MPBaseInterstitialAdapter *)adapter;
- (void)interstitialWillLeaveApplicationForAdapter:(MPBaseInterstitialAdapter *)adapter;

@end

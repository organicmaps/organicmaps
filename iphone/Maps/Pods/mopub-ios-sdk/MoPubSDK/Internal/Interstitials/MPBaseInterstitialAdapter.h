//
//  MPBaseInterstitialAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class CLLocation;
@class MPAdConfiguration;
@class MPAdTargeting;

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

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting;
- (void)_getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting;

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
- (void)interstitialDidReceiveImpressionEventForAdapter:(MPBaseInterstitialAdapter *)adapter;

@end

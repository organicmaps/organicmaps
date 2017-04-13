//
//  MPInstanceProvider.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPGlobal.h"
#import "MPCoreInstanceProvider.h"

// MPWebView
@class MPWebView;
@protocol MPWebViewDelegate;

// Banners
@class MPBannerAdManager;
@protocol MPBannerAdManagerDelegate;
@class MPBaseBannerAdapter;
@protocol MPBannerAdapterDelegate;
@class MPBannerCustomEvent;
@protocol MPBannerCustomEventDelegate;

// Interstitials
@class MPInterstitialAdManager;
@protocol MPInterstitialAdManagerDelegate;
@class MPBaseInterstitialAdapter;
@protocol MPInterstitialAdapterDelegate;
@class MPInterstitialCustomEvent;
@protocol MPInterstitialCustomEventDelegate;
@class MPHTMLInterstitialViewController;
@class MPMRAIDInterstitialViewController;
@protocol MPInterstitialViewControllerDelegate;

// Rewarded Video
@class MPRewardedVideoAdManager;
@class MPRewardedVideoAdapter;
@class MPRewardedVideoCustomEvent;
@protocol MPRewardedVideoAdapterDelegate;
@protocol MPRewardedVideoCustomEventDelegate;
@protocol MPRewardedVideoAdManagerDelegate;

// HTML Ads
@class MPAdWebViewAgent;
@protocol MPAdWebViewAgentDelegate;

// MRAID
@class MRController;
@protocol MRControllerDelegate;
@class MPClosableView;
@class MRBridge;
@protocol MRBridgeDelegate;
@protocol MPClosableViewDelegate;
@class MRBundleManager;
@class MRVideoPlayerManager;
@protocol MRVideoPlayerManagerDelegate;
@class MPMoviePlayerViewController;
@class MRNativeCommandHandler;
@protocol MRNativeCommandHandlerDelegate;

//Native
@protocol MPNativeCustomEventDelegate;
@class MPNativeCustomEvent;
@class MPNativeAdSource;
@protocol MPNativeAdSourceDelegate;
@class MPNativePositionSource;
@class MPStreamAdPlacementData;
@class MPStreamAdPlacer;
@class MPAdPositioning;
@protocol MPNativeAdRenderer;

@interface MPInstanceProvider : NSObject

+(instancetype)sharedProvider;
- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider;
- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider context:(id)context;

#pragma mark - Banners
- (MPBannerAdManager *)buildMPBannerAdManagerWithDelegate:(id<MPBannerAdManagerDelegate>)delegate;
- (MPBaseBannerAdapter *)buildBannerAdapterForConfiguration:(MPAdConfiguration *)configuration
                                                   delegate:(id<MPBannerAdapterDelegate>)delegate;
- (MPBannerCustomEvent *)buildBannerCustomEventFromCustomClass:(Class)customClass
                                                      delegate:(id<MPBannerCustomEventDelegate>)delegate;

#pragma mark - Interstitials
- (MPInterstitialAdManager *)buildMPInterstitialAdManagerWithDelegate:(id<MPInterstitialAdManagerDelegate>)delegate;
- (MPBaseInterstitialAdapter *)buildInterstitialAdapterForConfiguration:(MPAdConfiguration *)configuration
                                                               delegate:(id<MPInterstitialAdapterDelegate>)delegate;
- (MPInterstitialCustomEvent *)buildInterstitialCustomEventFromCustomClass:(Class)customClass
                                                                  delegate:(id<MPInterstitialCustomEventDelegate>)delegate;
- (MPHTMLInterstitialViewController *)buildMPHTMLInterstitialViewControllerWithDelegate:(id<MPInterstitialViewControllerDelegate>)delegate
                                                                        orientationType:(MPInterstitialOrientationType)type;
- (MPMRAIDInterstitialViewController *)buildMPMRAIDInterstitialViewControllerWithDelegate:(id<MPInterstitialViewControllerDelegate>)delegate
                                                                            configuration:(MPAdConfiguration *)configuration;

#pragma mark - Rewarded Video
- (MPRewardedVideoAdManager *)buildRewardedVideoAdManagerWithAdUnitID:(NSString *)adUnitID delegate:(id<MPRewardedVideoAdManagerDelegate>)delegate;
- (MPRewardedVideoAdapter *)buildRewardedVideoAdapterWithDelegate:(id<MPRewardedVideoAdapterDelegate>)delegate;
- (MPRewardedVideoCustomEvent *)buildRewardedVideoCustomEventFromCustomClass:(Class)customClass delegate:(id<MPRewardedVideoCustomEventDelegate>)delegate;


#pragma mark - HTML Ads
- (MPAdWebViewAgent *)buildMPAdWebViewAgentWithAdWebViewFrame:(CGRect)frame
                                                     delegate:(id<MPAdWebViewAgentDelegate>)delegate;

#pragma mark - MRAID
- (MPClosableView *)buildMRAIDMPClosableViewWithFrame:(CGRect)frame webView:(MPWebView *)webView delegate:(id<MPClosableViewDelegate>)delegate;
- (MRBundleManager *)buildMRBundleManager;
- (MRController *)buildBannerMRControllerWithFrame:(CGRect)frame delegate:(id<MRControllerDelegate>)delegate;
- (MRController *)buildInterstitialMRControllerWithFrame:(CGRect)frame delegate:(id<MRControllerDelegate>)delegate;
- (MRBridge *)buildMRBridgeWithWebView:(MPWebView *)webView delegate:(id<MRBridgeDelegate>)delegate;
- (MRVideoPlayerManager *)buildMRVideoPlayerManagerWithDelegate:(id<MRVideoPlayerManagerDelegate>)delegate;
- (MPMoviePlayerViewController *)buildMPMoviePlayerViewControllerWithURL:(NSURL *)URL;
- (MRNativeCommandHandler *)buildMRNativeCommandHandlerWithDelegate:(id<MRNativeCommandHandlerDelegate>)delegate;

#pragma mark - Native

- (MPNativeCustomEvent *)buildNativeCustomEventFromCustomClass:(Class)customClass
                                                      delegate:(id<MPNativeCustomEventDelegate>)delegate;
- (MPNativeAdSource *)buildNativeAdSourceWithDelegate:(id<MPNativeAdSourceDelegate>)delegate;
- (MPNativePositionSource *)buildNativePositioningSource;
- (MPStreamAdPlacementData *)buildStreamAdPlacementDataWithPositioning:(MPAdPositioning *)positioning;
- (MPStreamAdPlacer *)buildStreamAdPlacerWithViewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations;

@end

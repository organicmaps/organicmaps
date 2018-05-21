//
//  MPInstanceProvider.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPInstanceProvider.h"
#import "MPWebView.h"
#import "MPAdWebViewAgent.h"
#import "MPInterstitialAdManager.h"
#import "MPInterstitialCustomEventAdapter.h"
#import "MPHTMLInterstitialViewController.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPInterstitialCustomEvent.h"
#import "MPBaseBannerAdapter.h"
#import "MPBannerCustomEventAdapter.h"
#import "MPBannerCustomEvent.h"
#import "MPBannerAdManager.h"
#import "MPLogging.h"
#import "MRBundleManager.h"
#import "MRVideoPlayerManager.h"
#import <MediaPlayer/MediaPlayer.h>
#import "MRNativeCommandHandler.h"
#import "MRBridge.h"
#import "MRController.h"
#import "MPClosableView.h"
#import "MPRewardedVideoAdManager.h"
#import "MPRewardedVideoAdapter.h"
#import "MPRewardedVideoCustomEvent.h"

#if MP_HAS_NATIVE_PACKAGE
#import "MPNativeCustomEvent.h"
#import "MPNativeAdSource.h"
#import "MPNativePositionSource.h"
#import "MPStreamAdPlacementData.h"
#import "MPStreamAdPlacer.h"
#endif

@interface MPInstanceProvider ()

// A nested dictionary. The top-level dictionary maps Class objects to second-level dictionaries.
// The second level dictionaries map identifiers to singleton objects.
//
// An example:
//  {
//      SomeClass:
//      {
//          @"default": <singleton_a>
//          @"other_context": <singleton_b>
//      }
//  }
//
@property (nonatomic, strong) NSMutableDictionary *singletons;

@end


@implementation MPInstanceProvider

static MPInstanceProvider *sharedAdProvider = nil;

+ (instancetype)sharedProvider
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sharedAdProvider = [[self alloc] init];
    });

    return sharedAdProvider;
}

- (id)init
{
    self = [super init];
    if (self) {
        self.singletons = [NSMutableDictionary dictionary];
    }
    return self;
}

- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider
{
    return [self singletonForClass:klass provider:provider context:@"default"];
}

- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider context:(id)context
{
    id singleton = [[self.singletons objectForKey:klass] objectForKey:context];
    if (!singleton) {
        singleton = provider();
        NSMutableDictionary *singletonsForClass = [self.singletons objectForKey:klass];
        if (!singletonsForClass) {
            NSMutableDictionary *singletonsForContext = [NSMutableDictionary dictionaryWithObjectsAndKeys:singleton, context, nil];
            [self.singletons setObject:singletonsForContext forKey:(id<NSCopying>)klass];
        } else {
            [singletonsForClass setObject:singleton forKey:context];
        }
    }
    return singleton;
}

#pragma mark - Interstitials

- (MPInterstitialAdManager *)buildMPInterstitialAdManagerWithDelegate:(id<MPInterstitialAdManagerDelegate>)delegate
{
    return [(MPInterstitialAdManager *)[MPInterstitialAdManager alloc] initWithDelegate:delegate];
}


- (MPBaseInterstitialAdapter *)buildInterstitialAdapterForConfiguration:(MPAdConfiguration *)configuration
                                                               delegate:(id<MPInterstitialAdapterDelegate>)delegate
{
    if (configuration.customEventClass) {
        return [(MPInterstitialCustomEventAdapter *)[MPInterstitialCustomEventAdapter alloc] initWithDelegate:delegate];
    }

    return nil;
}

- (MPInterstitialCustomEvent *)buildInterstitialCustomEventFromCustomClass:(Class)customClass
                                                                  delegate:(id<MPInterstitialCustomEventDelegate>)delegate
{
    MPInterstitialCustomEvent *customEvent = [[customClass alloc] init];
    if (![customEvent isKindOfClass:[MPInterstitialCustomEvent class]]) {
        MPLogError(@"**** Custom Event Class: %@ does not extend MPInterstitialCustomEvent ****", NSStringFromClass(customClass));
        return nil;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
    if ([customEvent respondsToSelector:@selector(customEventDidUnload)]) {
        MPLogWarn(@"**** Custom Event Class: %@ implements the deprecated -customEventDidUnload method.  This is no longer called.  Use -dealloc for cleanup instead ****", NSStringFromClass(customClass));
    }
#pragma clang diagnostic pop
    customEvent.delegate = delegate;
    return customEvent;
}

- (MPHTMLInterstitialViewController *)buildMPHTMLInterstitialViewControllerWithDelegate:(id<MPInterstitialViewControllerDelegate>)delegate
                                                                        orientationType:(MPInterstitialOrientationType)type
{
    MPHTMLInterstitialViewController *controller = [[MPHTMLInterstitialViewController alloc] init];
    controller.delegate = delegate;
    controller.orientationType = type;
    return controller;
}

- (MPMRAIDInterstitialViewController *)buildMPMRAIDInterstitialViewControllerWithDelegate:(id<MPInterstitialViewControllerDelegate>)delegate
                                                                            configuration:(MPAdConfiguration *)configuration
{
    MPMRAIDInterstitialViewController *controller = [[MPMRAIDInterstitialViewController alloc] initWithAdConfiguration:configuration];
    controller.delegate = delegate;
    return controller;
}

#pragma mark - Rewarded Video

- (MPRewardedVideoAdManager *)buildRewardedVideoAdManagerWithAdUnitID:(NSString *)adUnitID delegate:(id<MPRewardedVideoAdManagerDelegate>)delegate
{
    return [[MPRewardedVideoAdManager alloc] initWithAdUnitID:adUnitID delegate:delegate];
}

- (MPRewardedVideoAdapter *)buildRewardedVideoAdapterWithDelegate:(id<MPRewardedVideoAdapterDelegate>)delegate
{
    return [[MPRewardedVideoAdapter alloc] initWithDelegate:delegate];
}

- (MPRewardedVideoCustomEvent *)buildRewardedVideoCustomEventFromCustomClass:(Class)customClass delegate:(id<MPRewardedVideoCustomEventDelegate>)delegate
{
    MPRewardedVideoCustomEvent *customEvent = [[customClass alloc] init];

    if (![customEvent isKindOfClass:[MPRewardedVideoCustomEvent class]]) {
        MPLogError(@"**** Custom Event Class: %@ does not extend MPRewardedVideoCustomEvent ****", NSStringFromClass(customClass));
        return nil;
    }

    customEvent.delegate = delegate;
    return customEvent;
}

#pragma mark - HTML Ads

- (MPAdWebViewAgent *)buildMPAdWebViewAgentWithAdWebViewFrame:(CGRect)frame delegate:(id<MPAdWebViewAgentDelegate>)delegate
{
    return [[MPAdWebViewAgent alloc] initWithAdWebViewFrame:frame delegate:delegate];
}

#pragma mark - MRAID

- (MPClosableView *)buildMRAIDMPClosableViewWithFrame:(CGRect)frame webView:(MPWebView *)webView delegate:(id<MPClosableViewDelegate>)delegate
{
    MPClosableView *adView = [[MPClosableView alloc] initWithFrame:frame closeButtonType:MPClosableViewCloseButtonTypeTappableWithImage];
    adView.delegate = delegate;
    adView.clipsToBounds = YES;
    webView.frame = adView.bounds;
    [adView addSubview:webView];
    return adView;
}

- (MRBundleManager *)buildMRBundleManager
{
    return [MRBundleManager sharedManager];
}

- (MRController *)buildBannerMRControllerWithFrame:(CGRect)frame delegate:(id<MRControllerDelegate>)delegate
{
    return [self buildMRControllerWithFrame:frame placementType:MRAdViewPlacementTypeInline delegate:delegate];
}

- (MRController *)buildInterstitialMRControllerWithFrame:(CGRect)frame delegate:(id<MRControllerDelegate>)delegate
{
    return [self buildMRControllerWithFrame:frame placementType:MRAdViewPlacementTypeInterstitial delegate:delegate];
}

- (MRController *)buildMRControllerWithFrame:(CGRect)frame placementType:(MRAdViewPlacementType)placementType delegate:(id<MRControllerDelegate>)delegate
{
    MRController *controller = [[MRController alloc] initWithAdViewFrame:frame adPlacementType:placementType];
    controller.delegate = delegate;
    return controller;
}

- (MRBridge *)buildMRBridgeWithWebView:(MPWebView *)webView delegate:(id<MRBridgeDelegate>)delegate
{
    MRBridge *bridge = [[MRBridge alloc] initWithWebView:webView];
    bridge.delegate = delegate;
    bridge.shouldHandleRequests = YES;
    return bridge;
}

- (MRVideoPlayerManager *)buildMRVideoPlayerManagerWithDelegate:(id<MRVideoPlayerManagerDelegate>)delegate
{
    return [[MRVideoPlayerManager alloc] initWithDelegate:delegate];
}

- (MPMoviePlayerViewController *)buildMPMoviePlayerViewControllerWithURL:(NSURL *)URL
{
    // ImageContext used to avoid CGErrors
    // http://stackoverflow.com/questions/13203336/iphone-mpmovieplayerviewcontroller-cgcontext-errors/14669166#14669166
    UIGraphicsBeginImageContext(CGSizeMake(1,1));
    MPMoviePlayerViewController *playerViewController = [[MPMoviePlayerViewController alloc] initWithContentURL:URL];
    UIGraphicsEndImageContext();

    return playerViewController;
}

- (MRNativeCommandHandler *)buildMRNativeCommandHandlerWithDelegate:(id<MRNativeCommandHandlerDelegate>)delegate
{
    return [[MRNativeCommandHandler alloc] initWithDelegate:delegate];
}

#pragma mark - Native

#if MP_HAS_NATIVE_PACKAGE

- (MPNativeCustomEvent *)buildNativeCustomEventFromCustomClass:(Class)customClass
                                                      delegate:(id<MPNativeCustomEventDelegate>)delegate
{
    MPNativeCustomEvent *customEvent = [[customClass alloc] init];
    if (![customEvent isKindOfClass:[MPNativeCustomEvent class]]) {
        MPLogError(@"**** Custom Event Class: %@ does not extend MPNativeCustomEvent ****", NSStringFromClass(customClass));
        return nil;
    }
    customEvent.delegate = delegate;
    return customEvent;
}

- (MPNativeAdSource *)buildNativeAdSourceWithDelegate:(id<MPNativeAdSourceDelegate>)delegate
{
    MPNativeAdSource *source = [MPNativeAdSource source];
    source.delegate = delegate;
    return source;
}

- (MPNativePositionSource *)buildNativePositioningSource
{
    return [[MPNativePositionSource alloc] init];
}

- (MPStreamAdPlacementData *)buildStreamAdPlacementDataWithPositioning:(MPAdPositioning *)positioning
{
    MPStreamAdPlacementData *placementData = [[MPStreamAdPlacementData alloc] initWithPositioning:positioning];
    return placementData;
}

- (MPStreamAdPlacer *)buildStreamAdPlacerWithViewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    return [MPStreamAdPlacer placerWithViewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
}

#endif

@end


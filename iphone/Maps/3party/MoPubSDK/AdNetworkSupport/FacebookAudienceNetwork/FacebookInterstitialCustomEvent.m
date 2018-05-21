//
//  FacebookInterstitialCustomEvent.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookInterstitialCustomEvent.h"

#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPRealTimeTimer.h"

//Timer to record the expiration interval
#define FB_ADS_EXPIRATION_INTERVAL  3600


@interface MPInstanceProvider (FacebookInterstitials)

- (FBInterstitialAd *)buildFBInterstitialAdWithPlacementID:(NSString *)placementID
                                                  delegate:(id<FBInterstitialAdDelegate>)delegate;

@end

@implementation MPInstanceProvider (FacebookInterstitials)

- (FBInterstitialAd *)buildFBInterstitialAdWithPlacementID:(NSString *)placementID
                                                  delegate:(id<FBInterstitialAdDelegate>)delegate
{
    FBInterstitialAd *interstitialAd = [[FBInterstitialAd alloc] initWithPlacementID:placementID];
    interstitialAd.delegate = delegate;
    return interstitialAd;
}

@end

@interface FacebookInterstitialCustomEvent () <FBInterstitialAdDelegate>

@property (nonatomic, strong) FBInterstitialAd *fbInterstitialAd;
@property (nonatomic, strong) MPRealTimeTimer *expirationTimer;
@property (nonatomic, assign) BOOL hasTrackedImpression;

@end

@implementation FacebookInterstitialCustomEvent

@synthesize hasTrackedImpression = _hasTrackedImpression;

- (void)requestInterstitialWithCustomEventInfo:(NSDictionary *)info
{
    [self requestInterstitialWithCustomEventInfo:info adMarkup:nil];
}

- (void)requestInterstitialWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    if (![info objectForKey:@"placement_id"]) {
        MPLogError(@"Placement ID is required for Facebook interstitial ad");
        [self.delegate interstitialCustomEvent:self didFailToLoadAdWithError:nil];
        return;
    }

    self.fbInterstitialAd =
    [[MPInstanceProvider sharedProvider] buildFBInterstitialAdWithPlacementID:[info objectForKey:@"placement_id"]
                                                                     delegate:self];
    [FBAdSettings setMediationService:[NSString stringWithFormat:@"MOPUB_%@", MP_SDK_VERSION]];
    
    // Load the advanced bid payload.
    if (adMarkup != nil) {
        MPLogInfo(@"Loading Facebook interstitial ad markup");
        [self.fbInterstitialAd loadAdWithBidPayload:adMarkup];
    }
    // Request a interstitial ad.
    else {
        MPLogInfo(@"Requesting Facebook interstitial ad");
        [self.fbInterstitialAd loadAd];
    }
}

- (void)showInterstitialFromRootViewController:(UIViewController *)controller {
    if (!self.fbInterstitialAd || !self.fbInterstitialAd.isAdValid) {
        MPLogError(@"Facebook interstitial ad was not loaded");
        [self.delegate interstitialCustomEventDidExpire:self];
    } else {
        MPLogInfo(@"Facebook interstitial ad will be presented");
        [self.delegate interstitialCustomEventWillAppear:self];
        [self.fbInterstitialAd showAdFromRootViewController:controller];
        MPLogInfo(@"Facebook interstitial ad was presented");
        [self.delegate interstitialCustomEventDidAppear:self];
    }
}

- (void)dealloc
{
    _fbInterstitialAd.delegate = nil;
}

#pragma mark FBInterstitialAdDelegate methods

- (void)interstitialAdDidLoad:(FBInterstitialAd *)interstitialAd
{
    MPLogInfo(@"Facebook intersitital ad was loaded. Can present now");
    [self.delegate interstitialCustomEvent:self didLoadAd:interstitialAd];
    
    // introduce timer for 1 hour as per caching logic introduced by FB
    
    __weak __typeof__(self) weakSelf = self;
    self.expirationTimer = [[MPRealTimeTimer alloc] initWithInterval:FB_ADS_EXPIRATION_INTERVAL block:^(MPRealTimeTimer *timer){
        __strong __typeof__(weakSelf) strongSelf = weakSelf;
        if (strongSelf && !strongSelf.hasTrackedImpression) {
            [self.delegate interstitialCustomEventDidExpire:self];
            MPLogInfo(@"Facebook intersitital ad expired as per the audience network's caching policy");
            //Delete the cached objects
            self.fbInterstitialAd = nil;
        }
        [strongSelf.expirationTimer invalidate];
    }];
    [self.expirationTimer scheduleNow];
    
}

- (void)interstitialAdWillLogImpression:(FBInterstitialAd *)interstitialAd
{
    MPLogInfo(@"Facebook intersitital ad is logging impressions for interstitials");
    //set the tracker to true when the ad is shown on the screen. So that the timer is invalidated.
    _hasTrackedImpression = true;
    [self.expirationTimer invalidate];
}

- (void)interstitialAd:(FBInterstitialAd *)interstitialAd didFailWithError:(NSError *)error
{
    MPLogInfo(@"Facebook intersitital ad failed to load with error: %@", error.localizedDescription);
    [self.delegate interstitialCustomEvent:self didFailToLoadAdWithError:nil];
}

- (void)interstitialAdDidClick:(FBInterstitialAd *)interstitialAd
{
    MPLogInfo(@"Facebook interstitial ad was clicked");
    [self.delegate interstitialCustomEventDidReceiveTapEvent:self];
}

- (void)interstitialAdDidClose:(FBInterstitialAd *)interstitialAd
{
    MPLogInfo(@"Facebook interstitial ad was closed");
    [self.delegate interstitialCustomEventDidDisappear:self];
}

- (void)interstitialAdWillClose:(FBInterstitialAd *)interstitialAd
{
    MPLogInfo(@"Facebook interstitial ad will close");
    [self.delegate interstitialCustomEventWillDisappear:self];
}

@end

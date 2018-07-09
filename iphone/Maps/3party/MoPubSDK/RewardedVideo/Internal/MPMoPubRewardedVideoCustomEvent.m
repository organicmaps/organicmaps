//
//  MPMoPubRewardedVideoCustomEvent.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPMoPubRewardedVideoCustomEvent.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPRewardedVideoReward.h"
#import "MPAdConfiguration.h"
#import "MPRewardedVideoAdapter.h"
#import "MPRewardedVideoReward.h"
#import "MPRewardedVideoError.h"

@interface MPMoPubRewardedVideoCustomEvent() <MPInterstitialViewControllerDelegate>

@property (nonatomic) MPMRAIDInterstitialViewController *interstitial;
@property (nonatomic) BOOL adAvailable;

@end

@implementation MPMoPubRewardedVideoCustomEvent

@dynamic delegate;

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info
{
    MPLogInfo(@"Loading MoPub rewarded video");
    self.interstitial = [[MPInstanceProvider sharedProvider] buildMPMRAIDInterstitialViewControllerWithDelegate:self
                                                                                                  configuration:[self.delegate configuration]];

    [self.interstitial setCloseButtonStyle:MPInterstitialCloseButtonStyleAlwaysHidden];
    [self.interstitial startLoading];
}

- (BOOL)hasAdAvailable
{
    return self.adAvailable;
}

- (void)handleAdPlayedForCustomEventNetwork
{
    // no-op
}

- (void)handleCustomEventInvalidated
{
    // no-op
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController
{
    if ([self hasAdAvailable]) {
        [self.interstitial presentInterstitialFromViewController:viewController];
    } else {
        MPLogInfo(@"Failed to show MoPub rewarded video");
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToPlayForCustomEvent:self error:error];
    }
}

#pragma mark - MPMRAIDInterstitialViewControllerDelegate

- (void)interstitialDidLoadAd:(MPInterstitialViewController *)interstitial
{
    MPLogInfo(@"MoPub rewarded video did load");
    self.adAvailable = YES;
    [self.delegate rewardedVideoDidLoadAdForCustomEvent:self];
}

- (void)interstitialDidAppear:(MPInterstitialViewController *)interstitial
{
    MPLogInfo(@"MoPub rewarded video did appear");
    [self.delegate rewardedVideoDidAppearForCustomEvent:self];
}

- (void)interstitialWillAppear:(MPInterstitialViewController *)interstitial
{
    MPLogInfo(@"MoPub rewarded video will appear");
    [self.delegate rewardedVideoWillAppearForCustomEvent:self];
}

- (void)interstitialDidFailToLoadAd:(MPInterstitialViewController *)interstitial
{
    MPLogInfo(@"MoPub rewarded video failed to load");
    self.adAvailable = NO;
    [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:nil];
}

- (void)interstitialWillDisappear:(MPInterstitialViewController *)interstitial
{
    [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
}

- (void)interstitialDidDisappear:(MPInterstitialViewController *)interstitial
{
    self.adAvailable = NO;
    [self.delegate rewardedVideoDidDisappearForCustomEvent:self];

    // Get rid of the interstitial view controller when done with it so we don't hold on longer than needed
    self.interstitial = nil;
}

- (void)interstitialDidReceiveTapEvent:(MPInterstitialViewController *)interstitial
{
    [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
}

- (void)interstitialWillLeaveApplication:(MPInterstitialViewController *)interstitial
{
    [self.delegate rewardedVideoWillLeaveApplicationForCustomEvent:self];
}

- (void)interstitialRewardedVideoEnded
{
    MPLogInfo(@"MoPub rewarded video finished playing.");
    [self.delegate rewardedVideoShouldRewardUserForCustomEvent:self reward:[self configuration].selectedReward];
}

#pragma mark - MPPrivateRewardedVideoCustomEventDelegate
- (NSString *)adUnitId
{
    return [self.delegate adUnitId];
}

- (MPAdConfiguration *)configuration
{
    return [self.delegate configuration];
}

@end

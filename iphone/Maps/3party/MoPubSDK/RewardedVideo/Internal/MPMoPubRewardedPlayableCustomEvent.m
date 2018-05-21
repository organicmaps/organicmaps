//
//  MPMoPubRewardedPlayableCustomEvent.m
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPMoPubRewardedPlayableCustomEvent.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPAdConfiguration.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPRewardedVideoError.h"
#import "MPCountdownTimerView.h"

const NSTimeInterval kDefaultCountdownTimerIntervalInSeconds = 30;

@interface MPMoPubRewardedPlayableCustomEvent() <MPInterstitialViewControllerDelegate>
@property (nonatomic, assign) BOOL adAvailable;
@property (nonatomic, strong) MPMRAIDInterstitialViewController *interstitial;
@property (nonatomic, strong) MPCountdownTimerView *timerView;
@property (nonatomic, assign) BOOL userRewarded;
@end

@implementation MPMoPubRewardedPlayableCustomEvent

- (void)dealloc {
    [_timerView stopAndSignalCompletion:NO];
}

// Lazy initialization property for the MRAID interstitial.
- (MPMRAIDInterstitialViewController *)interstitial {
    if (_interstitial == nil) {
        _interstitial = [[MPMRAIDInterstitialViewController alloc] initWithAdConfiguration:self.delegate.configuration];
    }

    return _interstitial;
}

// Retrieves a valid countdown duration to use for the timer. In the event that `rewardedPlayableDuration`
// from `MPAdConfiguration` is less than zero, the default value `kDefaultCountdownTimerIntervalInSeconds`
// will be used instead.
- (NSTimeInterval)countdownDuration {
    NSTimeInterval duration = self.delegate.configuration.rewardedPlayableDuration;
    if (duration <= 0) {
        duration = kDefaultCountdownTimerIntervalInSeconds;
    }

    return duration;
}

// Shows the native close button and deallocates the countdown timer since it will no
// longer be used.
- (void)showCloseButton {
    [self.interstitial setCloseButtonStyle:MPInterstitialCloseButtonStyleAlwaysVisible];
    [self.timerView removeFromSuperview];
    self.timerView = nil;
}

// Only reward the user once; either by countdown timer elapsing or rewarding on click
// (if configured).
- (void)rewardUserWithConfiguration:(MPAdConfiguration *)configuration timerHasElapsed:(BOOL)hasElasped  {
    if (!self.userRewarded && (hasElasped || configuration.rewardedPlayableShouldRewardOnClick)) {
        MPLogInfo(@"MoPub rewarded playable user rewarded.");

        [self.delegate rewardedVideoShouldRewardUserForCustomEvent:self reward:configuration.selectedReward];
        self.userRewarded = YES;
    }
}

#pragma mark - MPRewardedVideoCustomEvent

@dynamic delegate;

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info {
    MPLogInfo(@"Loading MoPub rewarded playable");
    self.interstitial.delegate = self;

    [self.interstitial setCloseButtonStyle:MPInterstitialCloseButtonStyleAlwaysHidden];
    [self.interstitial startLoading];
}

- (BOOL)hasAdAvailable {
    return self.adAvailable;
}

- (void)handleAdPlayedForCustomEventNetwork {
    // no-op
}

- (void)handleCustomEventInvalidated {
    // no-op
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController {
    if (self.hasAdAvailable) {
        // Add the countdown timer to the interstitial and start the timer.
        self.timerView = [[MPCountdownTimerView alloc] initWithFrame:viewController.view.bounds duration:self.countdownDuration];
        [self.interstitial.view addSubview:self.timerView];

        __weak __typeof__(self) weakSelf = self;
        [self.timerView startWithTimerCompletion:^(BOOL hasElapsed) {
            [weakSelf rewardUserWithConfiguration:self.configuration timerHasElapsed:hasElapsed];
            [weakSelf showCloseButton];
        }];

        [self.interstitial presentInterstitialFromViewController:viewController];
    }
    else {
        MPLogInfo(@"Failed to show MoPub rewarded playable");
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToPlayForCustomEvent:self error:error];
        [self showCloseButton];
    }
}

#pragma mark - MPInterstitialViewControllerDelegate

- (void)interstitialDidLoadAd:(MPInterstitialViewController *)interstitial {
    MPLogInfo(@"MoPub rewarded playable did load");
    self.adAvailable = YES;
    [self.delegate rewardedVideoDidLoadAdForCustomEvent:self];
}

- (void)interstitialDidAppear:(MPInterstitialViewController *)interstitial {
    MPLogInfo(@"MoPub rewarded playable did appear");
    [self.delegate rewardedVideoDidAppearForCustomEvent:self];
}

- (void)interstitialWillAppear:(MPInterstitialViewController *)interstitial {
    MPLogInfo(@"MoPub rewarded playable will appear");
    [self.delegate rewardedVideoWillAppearForCustomEvent:self];
}

- (void)interstitialDidFailToLoadAd:(MPInterstitialViewController *)interstitial {
    MPLogInfo(@"MoPub rewarded playable failed to load");
    self.adAvailable = NO;
    [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:nil];
}

- (void)interstitialWillDisappear:(MPInterstitialViewController *)interstitial {
    [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
}

- (void)interstitialDidDisappear:(MPInterstitialViewController *)interstitial {
    self.adAvailable = NO;
    [self.timerView stopAndSignalCompletion:NO];
    [self.delegate rewardedVideoDidDisappearForCustomEvent:self];

    // Get rid of the interstitial view controller when done with it so we don't hold on longer than needed
    self.interstitial = nil;
}

- (void)interstitialDidReceiveTapEvent:(MPInterstitialViewController *)interstitial {
    [self rewardUserWithConfiguration:self.configuration timerHasElapsed:NO];
    [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
}

- (void)interstitialWillLeaveApplication:(MPInterstitialViewController *)interstitial {
    [self.delegate rewardedVideoWillLeaveApplicationForCustomEvent:self];
}

#pragma mark - MPPrivateRewardedVideoCustomEventDelegate

- (NSString *)adUnitId {
    return [self.delegate adUnitId];
}

- (MPAdConfiguration *)configuration {
    return [self.delegate configuration];
}

@end

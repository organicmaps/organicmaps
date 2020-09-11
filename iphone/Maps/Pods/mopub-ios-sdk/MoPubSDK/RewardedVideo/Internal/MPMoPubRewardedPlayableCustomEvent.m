//
//  MPMoPubRewardedPlayableCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubRewardedPlayableCustomEvent.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPAdConfiguration.h"
#import "MPError.h"
#import "MPLogging.h"
#import "MPRewardedVideoError.h"
#import "MPCountdownTimerView.h"
#import "UIView+MPAdditions.h"

const NSTimeInterval kDefaultCountdownTimerIntervalInSeconds = 30;

@interface MPMoPubRewardedPlayableCustomEvent()

@property (nonatomic, assign) BOOL adAvailable;
@property (nonatomic, strong) MPMRAIDInterstitialViewController *interstitial;
@property (nonatomic, strong) MPCountdownTimerView *timerView;
@property (nonatomic, assign) BOOL userRewarded;
@property (nonatomic, assign) NSTimeInterval countdownDuration;

@end

@interface MPMoPubRewardedPlayableCustomEvent (MPInterstitialViewControllerDelegate) <MPInterstitialViewControllerDelegate>
@end

@implementation MPMoPubRewardedPlayableCustomEvent

- (NSString *)adUnitId {
    return [self.delegate adUnitId];
}

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

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup {
    MPAdConfiguration * configuration = self.delegate.configuration;
    MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(configuration.customEventClass) dspCreativeId:configuration.dspCreativeId dspName:nil], self.adUnitId);

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
    MPLogAdEvent([MPLogEvent adShowAttemptForAdapter:NSStringFromClass(self.class)], self.adUnitId);

    // Error handling block.
    __typeof__(self) __weak weakSelf = self;
    void (^onShowError)(NSError *) = ^(NSError * error) {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf != nil) {
            MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(strongSelf.class) error:error], strongSelf.adUnitId);

            [strongSelf.delegate rewardedVideoDidFailToPlayForCustomEvent:strongSelf error:error];
            [strongSelf showCloseButton];
        }
    };

    // No ad available to show.
    if (!self.hasAdAvailable) {
        NSError * error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        onShowError(error);
        return;
    }

    // Add the countdown timer to the interstitial and start the timer.
    self.timerView = [[MPCountdownTimerView alloc] initWithDuration:self.countdownDuration timerCompletion:^(BOOL hasElapsed) {
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf != nil) {
            [strongSelf rewardUserWithConfiguration:strongSelf.delegate.configuration timerHasElapsed:hasElapsed];
            [strongSelf showCloseButton];
        }
    }];
    [self.interstitial.view addSubview:self.timerView];

    NSArray *constraints = @[[self.timerView.topAnchor constraintEqualToAnchor:self.interstitial.view.mp_safeTopAnchor],
                             [self.timerView.rightAnchor constraintEqualToAnchor:self.interstitial.view.mp_safeRightAnchor]];
    [NSLayoutConstraint activateConstraints:constraints];
    self.timerView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.timerView start];

    [self.interstitial presentInterstitialFromViewController:viewController complete:^(NSError * error) {
        if (error != nil) {
            onShowError(error);
        }
        else {
            MPLogAdEvent([MPLogEvent adShowSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);
        }
    }];
}

@end

#pragma mark - MPInterstitialViewControllerDelegate

@implementation MPMoPubRewardedPlayableCustomEvent (MPInterstitialViewControllerDelegate)

- (void)interstitialDidLoadAd:(id<MPInterstitialViewController>)interstitial {
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);

    self.adAvailable = YES;
    [self.delegate rewardedVideoDidLoadAdForCustomEvent:self];
}

- (void)interstitialDidAppear:(id<MPInterstitialViewController>)interstitial {
    [self.delegate rewardedVideoDidAppearForCustomEvent:self];
}

- (void)interstitialWillAppear:(id<MPInterstitialViewController>)interstitial {
    [self.delegate rewardedVideoWillAppearForCustomEvent:self];
}

- (void)interstitialDidFailToLoadAd:(id<MPInterstitialViewController>)interstitial {
    NSString * message = [NSString stringWithFormat:@"Failed to load creative:\n%@", self.delegate.configuration.adResponseHTMLString];
    NSError * error = [NSError errorWithCode:MOPUBErrorAdapterFailedToLoadAd localizedDescription:message];
    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], self.adUnitId);

    self.adAvailable = NO;
    [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:nil];
}

- (void)interstitialWillDisappear:(id<MPInterstitialViewController>)interstitial {
    [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
}

- (void)interstitialDidDisappear:(id<MPInterstitialViewController>)interstitial {
    self.adAvailable = NO;
    [self.timerView stopAndSignalCompletion:NO];
    [self.delegate rewardedVideoDidDisappearForCustomEvent:self];

    // Get rid of the interstitial view controller when done with it so we don't hold on longer than needed
    self.interstitial = nil;
}

- (void)interstitialDidReceiveTapEvent:(id<MPInterstitialViewController>)interstitial {
    [self rewardUserWithConfiguration:self.delegate.configuration timerHasElapsed:NO];
    [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
}

- (void)interstitialWillLeaveApplication:(id<MPInterstitialViewController>)interstitial {
    [self.delegate rewardedVideoWillLeaveApplicationForCustomEvent:self];
}

@end

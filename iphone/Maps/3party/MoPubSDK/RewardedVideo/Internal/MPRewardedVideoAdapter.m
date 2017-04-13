//
//  MPRewardedVideoAdapter.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRewardedVideoAdapter.h"

#import "MPAdConfiguration.h"
#import "MPAnalyticsTracker.h"
#import "MPCoreInstanceProvider.h"
#import "MPRewardedVideoError.h"
#import "MPRewardedVideoCustomEvent.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "MPRewardedVideoReward.h"
#import "MPRewardedVideo+Internal.h"

static const NSString *kRewardedVideoApiVersion = @"1";

@interface MPRewardedVideoAdapter () <MPRewardedVideoCustomEventDelegate>

@property (nonatomic, strong) MPRewardedVideoCustomEvent *rewardedVideoCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPTimer *timeoutTimer;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;
// Once an ad successfully loads, we want to block sending more successful load events.
@property (nonatomic, assign) BOOL hasSuccessfullyLoaded;
// Since we only notify the application of one success per load, we also only notify the application of one expiration per success.
@property (nonatomic, assign) BOOL hasExpired;

@end

@implementation MPRewardedVideoAdapter

- (instancetype)initWithDelegate:(id<MPRewardedVideoAdapterDelegate>)delegate
{
    if (self = [super init]) {
        _delegate = delegate;
    }

    return self;
}

- (void)dealloc
{
    // The rewarded video system now no longer holds references to the custom event. The custom event may have a system
    // that holds extra references to the custom event. Let's tell the custom event that we no longer need it.
    [_rewardedVideoCustomEvent handleCustomEventInvalidated];

    [_timeoutTimer invalidate];

    // Make sure the custom event isn't released synchronously as objects owned by the custom event
    // may do additional work after a callback that results in dealloc being called
    [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:_rewardedVideoCustomEvent];
}

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration
{
    MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);

    self.configuration = configuration;

    self.rewardedVideoCustomEvent = [[MPInstanceProvider sharedProvider] buildRewardedVideoCustomEventFromCustomClass:configuration.customEventClass delegate:self];

    if (self.rewardedVideoCustomEvent) {
        [self startTimeoutTimer];
        [self.rewardedVideoCustomEvent requestRewardedVideoWithCustomEventInfo:configuration.customEventClassData];
    } else {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorInvalidCustomEvent userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
    }
}

- (BOOL)hasAdAvailable
{
    return [self.rewardedVideoCustomEvent hasAdAvailable];
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController
{
    [self.rewardedVideoCustomEvent presentRewardedVideoFromViewController:viewController];
}

- (void)handleAdPlayedForCustomEventNetwork
{
    [self.rewardedVideoCustomEvent handleAdPlayedForCustomEventNetwork];
}

#pragma mark - Private

- (void)startTimeoutTimer
{
    NSTimeInterval timeInterval = (self.configuration && self.configuration.adTimeoutInterval >= 0) ?
    self.configuration.adTimeoutInterval : REWARDED_VIDEO_TIMEOUT_INTERVAL;

    if (timeInterval > 0) {
        self.timeoutTimer = [[MPCoreInstanceProvider sharedProvider] buildMPTimerWithTimeInterval:timeInterval
                                                                                           target:self
                                                                                         selector:@selector(timeout)
                                                                                          repeats:NO];

        [self.timeoutTimer scheduleNow];
    }
}

- (void)timeout
{
    NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorTimeout userInfo:nil];
    [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
}

- (void)didStopLoading
{
    [self.timeoutTimer invalidate];
}

- (NSURL *)rewardedVideoCompletionUrlByAppendingClientParams
{
    NSString *finalCompletionUrlString = self.configuration.rewardedVideoCompletionUrl;
    if ([self.delegate respondsToSelector:@selector(rewardedVideoCustomerId)] && [self.delegate rewardedVideoCustomerId].length > 0) {
        // self.configuration.rewardedVideoCompletionUrl is already url encoded. Only the customer_id added by the client needs url encoding.
        NSString *urlEncodedCustomerId = [[self.delegate rewardedVideoCustomerId] mp_URLEncodedString];
        finalCompletionUrlString = [NSString stringWithFormat:@"%@&customer_id=%@", finalCompletionUrlString, urlEncodedCustomerId];
    }
    finalCompletionUrlString = [NSString stringWithFormat:@"%@&nv=%@&v=%@", finalCompletionUrlString, [MP_SDK_VERSION mp_URLEncodedString], kRewardedVideoApiVersion];

    if (self.configuration.selectedReward) {
        finalCompletionUrlString = [NSString stringWithFormat:@"%@&rcn=%@&rca=%i", finalCompletionUrlString, [self.configuration.selectedReward.currencyType mp_URLEncodedString], [self.configuration.selectedReward.amount intValue]];
    }

    return [NSURL URLWithString:finalCompletionUrlString];
}

#pragma mark - Metrics

- (void)trackImpression
{
    [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] trackImpressionForConfiguration:self.configuration];
}

- (void)trackClick
{
    [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] trackClickForConfiguration:self.configuration];
}

#pragma mark - MPRewardedVideoCustomEventDelegate

- (id<MPMediationSettingsProtocol>)instanceMediationSettingsForClass:(Class)aClass
{
    return [self.delegate instanceMediationSettingsForClass:aClass];
}

- (void)rewardedVideoDidLoadAdForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    // Don't report multiple successful loads. Backing ad networks may replenish their caches triggering multiple successful load
    // callbacks.
    if (self.hasSuccessfullyLoaded) {
        return;
    }

    self.hasSuccessfullyLoaded = YES;
    [self didStopLoading];
    [self.delegate rewardedVideoDidLoadForAdapter:self];
}

- (void)rewardedVideoDidFailToLoadAdForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent error:(NSError *)error
{
    // Detach the custom event from the adapter. An ad *may* end up, after some time, loading successfully
    // from the underlying network, but we don't want to bubble up the event to the application since we
    // are reporting a timeout here.
    [self.rewardedVideoCustomEvent handleCustomEventInvalidated];
    self.rewardedVideoCustomEvent = nil;

    [self didStopLoading];
    [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
}

- (void)rewardedVideoDidExpireForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    // Only allow one expire per custom event to match up with one successful load callback per custom event.
    if (self.hasExpired) {
        return;
    }

    self.hasExpired = YES;
    [self.delegate rewardedVideoDidExpireForAdapter:self];
}

- (void)rewardedVideoDidFailToPlayForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent error:(NSError *)error
{
    [self.delegate rewardedVideoDidFailToPlayForAdapter:self error:error];
}

- (void)rewardedVideoWillAppearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    [self.delegate rewardedVideoWillAppearForAdapter:self];
}

- (void)rewardedVideoDidAppearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    if ([self.rewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedImpression) {
        self.hasTrackedImpression = YES;
        [self trackImpression];
    }

    [self.delegate rewardedVideoDidAppearForAdapter:self];
}

- (void)rewardedVideoWillDisappearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    [self.delegate rewardedVideoWillDisappearForAdapter:self];
}

- (void)rewardedVideoDidDisappearForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    [self.delegate rewardedVideoDidDisappearForAdapter:self];
}

- (void)rewardedVideoWillLeaveApplicationForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    [self.delegate rewardedVideoWillLeaveApplicationForAdapter:self];
}

- (void)rewardedVideoDidReceiveTapEventForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    if ([self.rewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedClick) {
        self.hasTrackedClick = YES;
        [self trackClick];
    }

    [self.delegate rewardedVideoDidReceiveTapEventForAdapter:self];
}

- (void)rewardedVideoShouldRewardUserForCustomEvent:(MPRewardedVideoCustomEvent *)customEvent reward:(MPRewardedVideoReward *)reward
{
    if (self.configuration && self.configuration.rewardedVideoCompletionUrl) {
        // server to server callback
        [[MPRewardedVideo sharedInstance] startRewardedVideoConnectionWithUrl:[self rewardedVideoCompletionUrlByAppendingClientParams]];
    } else {
        // server to server not enabled. It uses client side rewarding.
        if (self.configuration) {
            MPRewardedVideoReward *mopubConfiguredReward = self.configuration.selectedReward;
            // If reward is set in adConfig, use reward that's set in adConfig.
            // Currency type has to be defined in mopubConfiguredReward in order to use mopubConfiguredReward.
            if (mopubConfiguredReward && mopubConfiguredReward.currencyType != kMPRewardedVideoRewardCurrencyTypeUnspecified){
                reward = mopubConfiguredReward;
            }
        }

        if (reward) {
            [self.delegate rewardedVideoShouldRewardUserForAdapter:self reward:reward];
        }
    }
}

- (NSString *)customerIdForRewardedVideoCustomEvent:(MPRewardedVideoCustomEvent *)customEvent
{
    if ([self.delegate respondsToSelector:@selector(rewardedVideoCustomerId)]) {
        return [self.delegate rewardedVideoCustomerId];
    }

    return nil;
}

#pragma mark - MPPrivateRewardedVideoCustomEventDelegate

- (NSString *)adUnitId
{
    if ([self.delegate respondsToSelector:@selector(rewardedVideoAdUnitId)]) {
        return [self.delegate rewardedVideoAdUnitId];
    }
    return nil;
}

- (MPAdConfiguration *)configuration
{
    return _configuration;
}

@end

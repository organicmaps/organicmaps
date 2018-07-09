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
#import "MPConstants.h"
#import "MPMoPubRewardedVideoCustomEvent.h"
#import "MPMoPubRewardedPlayableCustomEvent.h"
#import "MPRealTimeTimer.h"
#import "NSString+MPAdditions.h"

static const NSString *kRewardedVideoApiVersion = @"1";
static const NSUInteger kExcessiveCustomDataLength = 8196;

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
@property (nonatomic, strong) MPRealTimeTimer *expirationTimer;
@property (nonatomic, copy) NSString * urlEncodedCustomData;

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
        [self.rewardedVideoCustomEvent requestRewardedVideoWithCustomEventInfo:configuration.customEventClassData adMarkup:configuration.advancedBidPayload];
    } else {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorInvalidCustomEvent userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
    }
}

- (BOOL)hasAdAvailable
{
    return [self.rewardedVideoCustomEvent hasAdAvailable];
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController customData:(NSString *)customData
{
    NSUInteger customDataLength = customData.length;
    // Only persist the custom data field if it's non-empty and there is a server-to-server
    // callback URL. The persisted custom data will be url encoded.
    if (customDataLength > 0 && self.configuration.rewardedVideoCompletionUrl != nil) {
        // Warn about excessive custom data length, but allow the custom data to be sent anyway
        if (customDataLength > kExcessiveCustomDataLength) {
            MPLogWarn(@"Custom data length %ld exceeds the receommended maximum length of %ld characters.", customDataLength, kExcessiveCustomDataLength);
        }

        self.urlEncodedCustomData = [customData mp_URLEncodedString];
    }

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

    if (self.configuration.selectedReward && ![self.configuration.selectedReward.currencyType isEqualToString:kMPRewardedVideoRewardCurrencyTypeUnspecified]) {
        finalCompletionUrlString = [NSString stringWithFormat:@"%@&rcn=%@&rca=%i", finalCompletionUrlString, [self.configuration.selectedReward.currencyType mp_URLEncodedString], [self.configuration.selectedReward.amount intValue]];
    }

    // Append URL encoded custom event class name
    if (self.rewardedVideoCustomEvent != nil) {
        NSString * networkClassName = [NSStringFromClass([self.rewardedVideoCustomEvent class]) mp_URLEncodedString];
        finalCompletionUrlString = [NSString stringWithFormat:@"%@&cec=%@", finalCompletionUrlString, networkClassName];
    }

    // Append URL encoded custom data
    if (self.urlEncodedCustomData != nil) {
        finalCompletionUrlString = [NSString stringWithFormat:@"%@&rcd=%@", finalCompletionUrlString, self.urlEncodedCustomData];
    }

    return [NSURL URLWithString:finalCompletionUrlString];
}

#pragma mark - Metrics

- (void)trackImpression
{
    [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] trackImpressionForConfiguration:self.configuration];
    self.hasTrackedImpression = YES;
    [self.expirationTimer invalidate];
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

    // Check for MoPub-specific custom events before setting the timer
    if ([customEvent isKindOfClass:[MPMoPubRewardedVideoCustomEvent class]]
        || [customEvent isKindOfClass:[MPMoPubRewardedPlayableCustomEvent class]]) {
        // Set up timer for expiration
        __weak __typeof__(self) weakSelf = self;
        self.expirationTimer = [[MPRealTimeTimer alloc] initWithInterval:[MPConstants adsExpirationInterval] block:^(MPRealTimeTimer *timer){
            __strong __typeof__(weakSelf) strongSelf = weakSelf;
            if (strongSelf && !strongSelf.hasTrackedImpression) {
                [strongSelf rewardedVideoDidExpireForCustomEvent:strongSelf.rewardedVideoCustomEvent];
            }
            [strongSelf.expirationTimer invalidate];
        }];
        [self.expirationTimer scheduleNow];
    }
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
    if (self.configuration) {
        // Send server to server callback if available
        if (self.configuration.rewardedVideoCompletionUrl) {
            [[MPRewardedVideo sharedInstance] startRewardedVideoConnectionWithUrl:[self rewardedVideoCompletionUrlByAppendingClientParams]];
        }

        MPRewardedVideoReward *mopubConfiguredReward = self.configuration.selectedReward;

        // If reward is set in adConfig, use reward that's set in adConfig.
        // Currency type has to be defined in mopubConfiguredReward in order to use mopubConfiguredReward.
        if (mopubConfiguredReward && ![mopubConfiguredReward.currencyType isEqualToString:kMPRewardedVideoRewardCurrencyTypeUnspecified]) {
            reward = mopubConfiguredReward;
        }
    }

    // Notify client with the reward if present.
    if (reward) {
        [self.delegate rewardedVideoShouldRewardUserForAdapter:self reward:reward];
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

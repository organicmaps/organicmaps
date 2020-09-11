//
//  MPRewardedVideoAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideoAdapter.h"

#import "MPAdConfiguration.h"
#import "MPAdServerURLBuilder.h"
#import "MPAdTargeting.h"
#import "MPAnalyticsTracker.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPRewardedVideoError.h"
#import "MPRewardedVideoCustomEvent.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "MPRewardedVideoReward.h"
#import "MPRewardedVideo+Internal.h"
#import "MPConstants.h"
#import "MPMoPubRewardedVideoCustomEvent.h"
#import "MPMoPubRewardedPlayableCustomEvent.h"
#import "MPRealTimeTimer.h"
#import "MPVASTInterstitialCustomEvent.h"
#import "NSString+MPAdditions.h"

static const NSUInteger kExcessiveCustomDataLength = 8196;

@interface MPRewardedVideoAdapter () <MPRewardedVideoCustomEventDelegate>

@property (nonatomic, strong) id<MPRewardedVideoCustomEvent> rewardedVideoCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPTimer *timeoutTimer;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;
// Once an ad successfully loads, we want to block sending more successful load events.
@property (nonatomic, assign) BOOL hasSuccessfullyLoaded;
// Since we only notify the application of one success per load, we also only notify the application of one expiration per success.
@property (nonatomic, assign) BOOL hasExpired;
@property (nonatomic, strong) MPRealTimeTimer *expirationTimer;
@property (nonatomic, copy) NSString * customData;

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

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting
{
    MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);

    self.configuration = configuration;
    id<MPRewardedVideoCustomEvent> customEvent = [[configuration.customEventClass alloc] init];
    if (![customEvent conformsToProtocol:@protocol(MPRewardedVideoCustomEvent)]) {
        NSError * error = [NSError customEventClass:configuration.customEventClass doesNotInheritFrom:MPRewardedVideoCustomEvent.class];
        MPLogEvent([MPLogEvent error:error message:nil]);
        [self.delegate rewardedVideoDidFailToLoadForAdapter:nil error:error];
        return;
    }
    customEvent.delegate = self;
    customEvent.localExtras = targeting.localExtras;

    self.rewardedVideoCustomEvent = customEvent;
    [self startTimeoutTimer];

    [self.rewardedVideoCustomEvent requestRewardedVideoWithCustomEventInfo:configuration.customEventClassData adMarkup:configuration.advancedBidPayload];
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
            MPLogInfo(@"Custom data length %lu exceeds the receommended maximum length of %lu characters.", (unsigned long)customDataLength, (unsigned long)kExcessiveCustomDataLength);
        }

        self.customData = customData;
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
        self.timeoutTimer = [MPTimer timerWithTimeInterval:timeInterval
                                                    target:self
                                                  selector:@selector(timeout)
                                                   repeats:NO];
        [self.timeoutTimer scheduleNow];
    }
}

- (void)timeout
{
    NSError * error = [NSError errorWithCode:MOPUBErrorAdRequestTimedOut localizedDescription:@"Rewarded video ad request timed out"];
    [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
    self.delegate = nil;
}

- (void)didStopLoading
{
    [self.timeoutTimer invalidate];
}

- (NSURL *)rewardedVideoCompletionUrlByAppendingClientParams
{
    NSString * sourceCompletionUrl = self.configuration.rewardedVideoCompletionUrl;
    NSString * customerId = ([self.delegate respondsToSelector:@selector(rewardedVideoCustomerId)] ? [self.delegate rewardedVideoCustomerId] : nil);
    MPRewardedVideoReward * reward = (self.configuration.selectedReward != nil && ![self.configuration.selectedReward.currencyType isEqualToString:kMPRewardedVideoRewardCurrencyTypeUnspecified] ? self.configuration.selectedReward : nil);
    NSString * customEventName = NSStringFromClass([self.rewardedVideoCustomEvent class]);

    return [MPAdServerURLBuilder rewardedCompletionUrl:sourceCompletionUrl
                                        withCustomerId:customerId
                                            rewardType:reward.currencyType
                                          rewardAmount:reward.amount
                                       customEventName:customEventName
                                        additionalData:self.customData];
}

#pragma mark - Metrics

- (void)trackImpression
{
    [[MPAnalyticsTracker sharedTracker] trackImpressionForConfiguration:self.configuration];
    self.hasTrackedImpression = YES;
    [self.expirationTimer invalidate];
    [self.delegate rewardedVideoDidReceiveImpressionEventForAdapter:self];
}

- (void)trackClick
{
    [[MPAnalyticsTracker sharedTracker] trackClickForConfiguration:self.configuration];
}

#pragma mark - MPRewardedVideoCustomEventDelegate

- (id<MPMediationSettingsProtocol>)instanceMediationSettingsForClass:(Class)aClass
{
    return [self.delegate instanceMediationSettingsForClass:aClass];
}

- (void)rewardedVideoDidLoadAdForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
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
    // Custom events for 3rd party SDK have their own timeout and expiration handling
    if ([customEvent isKindOfClass:[MPVASTInterstitialCustomEvent class]]
        || [customEvent isKindOfClass:[MPMoPubRewardedVideoCustomEvent class]]
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

- (void)rewardedVideoDidFailToLoadAdForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent error:(NSError *)error
{
    // Detach the custom event from the adapter. An ad *may* end up, after some time, loading successfully
    // from the underlying network, but we don't want to bubble up the event to the application since we
    // are reporting a timeout here.
    [self.rewardedVideoCustomEvent handleCustomEventInvalidated];
    self.rewardedVideoCustomEvent = nil;

    [self didStopLoading];
    [self.delegate rewardedVideoDidFailToLoadForAdapter:self error:error];
}

- (void)rewardedVideoDidExpireForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    // Only allow one expire per custom event to match up with one successful load callback per custom event.
    if (self.hasExpired) {
        return;
    }

    self.hasExpired = YES;
    [self.delegate rewardedVideoDidExpireForAdapter:self];
}

- (void)rewardedVideoDidFailToPlayForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent error:(NSError *)error
{
    [self.delegate rewardedVideoDidFailToPlayForAdapter:self error:error];
}

- (void)rewardedVideoWillAppearForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    [self.delegate rewardedVideoWillAppearForAdapter:self];
}

- (void)rewardedVideoDidAppearForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    if ([self.rewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedImpression) {
        [self trackImpression];
    }

    [self.delegate rewardedVideoDidAppearForAdapter:self];
}

- (void)rewardedVideoWillDisappearForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    [self.delegate rewardedVideoWillDisappearForAdapter:self];
}

- (void)rewardedVideoDidDisappearForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    [self.delegate rewardedVideoDidDisappearForAdapter:self];
}

- (void)rewardedVideoWillLeaveApplicationForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    [self.delegate rewardedVideoWillLeaveApplicationForAdapter:self];
}

- (void)rewardedVideoDidReceiveTapEventForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
{
    if ([self.rewardedVideoCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedClick) {
        self.hasTrackedClick = YES;
        [self trackClick];
    }

    [self.delegate rewardedVideoDidReceiveTapEventForAdapter:self];
}

- (void)rewardedVideoShouldRewardUserForCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent reward:(MPRewardedVideoReward *)reward
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

- (NSString *)customerIdForRewardedVideoCustomEvent:(id<MPRewardedVideoCustomEvent>)customEvent
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

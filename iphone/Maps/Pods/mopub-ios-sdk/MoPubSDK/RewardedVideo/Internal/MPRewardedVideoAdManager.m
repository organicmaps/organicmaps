//
//  MPRewardedVideoAdManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideoAdManager.h"

#import "MPAdServerCommunicator.h"
#import "MPAdServerURLBuilder.h"
#import "MPRewardedVideoAdapter.h"
#import "MPCoreInstanceProvider.h"
#import "MPRewardedVideoError.h"
#import "MPLogging.h"
#import "MPStopwatch.h"
#import "MoPub.h"
#import "NSMutableArray+MPAdditions.h"
#import "NSDate+MPAdditions.h"
#import "NSError+MPAdditions.h"

@interface MPRewardedVideoAdManager () <MPAdServerCommunicatorDelegate, MPRewardedVideoAdapterDelegate>

@property (nonatomic, strong) MPRewardedVideoAdapter *adapter;
@property (nonatomic, strong) MPAdServerCommunicator *communicator;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) NSMutableArray<MPAdConfiguration *> *remainingConfigurations;
@property (nonatomic, strong) NSURL *mostRecentlyLoadedURL;  // ADF-4286: avoid infinite ad reloads
@property (nonatomic, assign) BOOL loading;
@property (nonatomic, assign) BOOL playedAd;
@property (nonatomic, assign) BOOL ready;
@property (nonatomic, strong) MPStopwatch *loadStopwatch;

@end

@implementation MPRewardedVideoAdManager

- (instancetype)initWithAdUnitID:(NSString *)adUnitID delegate:(id<MPRewardedVideoAdManagerDelegate>)delegate
{
    if (self = [super init]) {
        _adUnitId = [adUnitID copy];
        _communicator = [[MPAdServerCommunicator alloc] initWithDelegate:self];
        _delegate = delegate;
        _loadStopwatch = MPStopwatch.new;
    }

    return self;
}

- (void)dealloc
{
    [_communicator cancel];
}

- (NSArray *)availableRewards
{
    return self.configuration.availableRewards;
}

- (MPRewardedVideoReward *)selectedReward
{
    return self.configuration.selectedReward;
}

- (Class)customEventClass
{
    return self.configuration.customEventClass;
}

- (BOOL)hasAdAvailable
{
    //An Ad is not ready or has expired.
    if (!self.ready) {
        return NO;
    }

    // If we've already played an ad, return NO since we allow one play per load.
    if (self.playedAd) {
        return NO;
    }
    return [self.adapter hasAdAvailable];
}

- (void)loadRewardedVideoAdWithCustomerId:(NSString *)customerId targeting:(MPAdTargeting *)targeting
{
    MPLogAdEvent(MPLogEvent.adLoadAttempt, self.adUnitId);

    // We will just tell the delegate that we have loaded an ad if we already have one ready. However, if we have already
    // played a video for this ad manager, we will go ahead and request another ad from the server so we aren't potentially
    // stuck playing ads from the same network for a prolonged period of time which could be unoptimal with respect to the waterfall.
    if (self.ready && !self.playedAd) {
        // If we already have an ad, do not set the customerId. We'll leave the customerId as the old one since the ad we currently have
        // may be tied to an older customerId.
        [self.delegate rewardedVideoDidLoadForAdManager:self];
    } else {
        // This has multiple behaviors. For ads that require us to set the customID: (outside of load), this will overwrite the ad's previously
        // set customerId. Other ads require customerId on presentation in which we will use this new id coming in when presenting the ad.
        self.customerId = customerId;
        self.targeting = targeting;
        [self loadAdWithURL:[MPAdServerURLBuilder URLWithAdUnitID:self.adUnitId targeting:targeting]];
    }
}

- (void)presentRewardedVideoAdFromViewController:(UIViewController *)viewController withReward:(MPRewardedVideoReward *)reward customData:(NSString *)customData
{
    MPLogAdEvent(MPLogEvent.adShowAttempt, self.adUnitId);

    // Don't allow the ad to be shown if it isn't ready.
    if (!self.ready) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdReady userInfo:@{ NSLocalizedDescriptionKey: @"Rewarded video ad view is not ready to be shown"}];
        MPLogInfo(@"%@ error: %@", NSStringFromSelector(_cmd), error.localizedDescription);
        [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
        return;
    }

    // If we've already played an ad, don't allow playing of another since we allow one play per load.
    if (self.playedAd) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorAdAlreadyPlayed userInfo:nil];
        [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
        return;
    }

    // No reward is specified
    if (reward == nil) {
        // Only a single currency; It should automatically select the only currency available.
        if (self.availableRewards.count == 1) {
            MPRewardedVideoReward * defaultReward = self.availableRewards[0];
            self.configuration.selectedReward = defaultReward;
        }
        // Unspecified rewards in a multicurrency situation are not allowed.
        else {
            NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoRewardSelected userInfo:nil];
            [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
            return;
        }
    }
    // Reward is specified
    else {
        // Verify that the reward exists in the list of available rewards. If it doesn't, fail to play the ad.
        if (![self.availableRewards containsObject:reward]) {
            NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorInvalidReward userInfo:nil];
            [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
            return;
        }
        // Reward passes validation, set it as selected.
        else {
            self.configuration.selectedReward = reward;
        }
    }

    [self.adapter presentRewardedVideoFromViewController:viewController customData:customData];
}

- (void)handleAdPlayedForCustomEventNetwork
{
    // We only need to notify the backing ad network if the ad is marked ready for display.
    if (self.ready) {
        [self.adapter handleAdPlayedForCustomEventNetwork];
    }
}

#pragma mark - Private

- (void)loadAdWithURL:(NSURL *)URL
{
    self.playedAd = NO;

    if (self.loading) {
        MPLogEvent([MPLogEvent error:NSError.adAlreadyLoading message:nil]);
        return;
    }

    self.loading = YES;
    self.mostRecentlyLoadedURL = URL;
    [self.communicator loadURL:URL];
}

- (void)fetchAdWithConfiguration:(MPAdConfiguration *)configuration {
    MPLogInfo(@"Rewarded video ad is fetching ad type: %@", configuration.adType);

    if (configuration.adUnitWarmingUp) {
        MPLogInfo(kMPWarmingUpErrorLogFormatWithAdUnitID, self.adUnitId);
        self.loading = NO;
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorAdUnitWarmingUp userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
        return;
    }

    if ([configuration.adType isEqualToString:kAdTypeClear]) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitId);
        self.loading = NO;
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
        return;
    }

    // Notify Ad Server of the adapter load. This is fire and forget.
    [self.communicator sendBeforeLoadUrlWithConfiguration:configuration];

    // Start the stopwatch for the adapter load.
    [self.loadStopwatch start];

    MPRewardedVideoAdapter *adapter = [[MPRewardedVideoAdapter alloc] initWithDelegate:self];

    if (adapter == nil) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorUnknown userInfo:nil];
        [self rewardedVideoDidFailToLoadForAdapter:nil error:error];
        return;
    }

    self.adapter = adapter;
    [self.adapter getAdWithConfiguration:configuration targeting:self.targeting];
}

#pragma mark - MPAdServerCommunicatorDelegate

- (void)communicatorDidReceiveAdConfigurations:(NSArray<MPAdConfiguration *> *)configurations
{
    self.remainingConfigurations = [configurations mutableCopy];
    self.configuration = [self.remainingConfigurations removeFirst];

    // There are no configurations to try. Consider this a clear response by the server.
    if (self.remainingConfigurations.count == 0 && self.configuration == nil) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitId);
        self.loading = NO;
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
        return;
    }

    [self fetchAdWithConfiguration:self.configuration];
}

- (void)communicatorDidFailWithError:(NSError *)error
{
    self.ready = NO;
    self.loading = NO;

    [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
}

- (BOOL)isFullscreenAd {
    return YES;
}

#pragma mark - MPRewardedVideoAdapterDelegate

- (id<MPMediationSettingsProtocol>)instanceMediationSettingsForClass:(Class)aClass
{
    for (id<MPMediationSettingsProtocol> settings in self.mediationSettings) {
        if ([settings isKindOfClass:aClass]) {
            return settings;
        }
    }

    return nil;
}

- (void)rewardedVideoDidLoadForAdapter:(MPRewardedVideoAdapter *)adapter
{
    self.remainingConfigurations = nil;
    self.ready = YES;
    self.loading = NO;

    // Record the end of the adapter load and send off the fire and forget after-load-url tracker.
    // Start the stopwatch for the adapter load.
    NSTimeInterval duration = [self.loadStopwatch stop];
    [self.communicator sendAfterLoadUrlWithConfiguration:self.configuration adapterLoadDuration:duration adapterLoadResult:MPAfterLoadResultAdLoaded];

    MPLogAdEvent(MPLogEvent.adDidLoad, self.adUnitId);
    [self.delegate rewardedVideoDidLoadForAdManager:self];
}

- (void)rewardedVideoDidFailToLoadForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error
{
    // Record the end of the adapter load and send off the fire and forget after-load-url tracker
    // with the appropriate error code result.
    NSTimeInterval duration = [self.loadStopwatch stop];
    MPAfterLoadResult result = (error.isAdRequestTimedOutError ? MPAfterLoadResultTimeout : (adapter == nil ? MPAfterLoadResultMissingAdapter : MPAfterLoadResultError));
    [self.communicator sendAfterLoadUrlWithConfiguration:self.configuration adapterLoadDuration:duration adapterLoadResult:result];

    // There are more ad configurations to try.
    if (self.remainingConfigurations.count > 0) {
        self.configuration = [self.remainingConfigurations removeFirst];
        [self fetchAdWithConfiguration:self.configuration];
    }
    // No more configurations to try. Send new request to Ads server to get more Ads.
    else if (self.configuration.nextURL != nil
             && [self.configuration.nextURL isEqual:self.mostRecentlyLoadedURL] == false) {
        self.ready = NO;
        self.loading = NO;
        [self loadAdWithURL:self.configuration.nextURL];
    }
    // No more configurations to try and no more pages to load.
    else {
        self.ready = NO;
        self.loading = NO;

        NSString *errorDescription = [NSString stringWithFormat:kMPClearErrorLogFormatWithAdUnitID, self.adUnitId];
        NSError * clearResponseError = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain
                                                           code:MPRewardedVideoAdErrorNoAdsAvailable
                                                       userInfo:@{NSLocalizedDescriptionKey: errorDescription}];
        MPLogAdEvent([MPLogEvent adFailedToLoadWithError:clearResponseError], self.adUnitId);
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:clearResponseError];
    }
}

- (void)rewardedVideoDidExpireForAdapter:(MPRewardedVideoAdapter *)adapter
{
    self.ready = NO;

    MPLogAdEvent([MPLogEvent adExpiredWithTimeInterval:MPConstants.adsExpirationInterval], self.adUnitId);
    [self.delegate rewardedVideoDidExpireForAdManager:self];
}

- (void)rewardedVideoDidFailToPlayForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error
{
    // Playback of the rewarded video failed; reset the internal played state
    // so that a new rewarded video ad can be loaded.
    self.ready = NO;
    self.playedAd = NO;

    MPLogAdEvent([MPLogEvent adShowFailedWithError:error], self.adUnitId);
    [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
}

- (void)rewardedVideoWillAppearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    MPLogAdEvent(MPLogEvent.adWillAppear, self.adUnitId);
    [self.delegate rewardedVideoWillAppearForAdManager:self];
}

- (void)rewardedVideoDidAppearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    MPLogAdEvent(MPLogEvent.adDidAppear, self.adUnitId);
    [self.delegate rewardedVideoDidAppearForAdManager:self];
}

- (void)rewardedVideoWillDisappearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    MPLogAdEvent(MPLogEvent.adWillDisappear, self.adUnitId);
    [self.delegate rewardedVideoWillDisappearForAdManager:self];
}

- (void)rewardedVideoDidDisappearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    // Successful playback of the rewarded video; reset the internal played state.
    self.ready = NO;
    self.playedAd = YES;

    MPLogAdEvent(MPLogEvent.adDidDisappear, self.adUnitId);
    [self.delegate rewardedVideoDidDisappearForAdManager:self];
}

- (void)rewardedVideoDidReceiveTapEventForAdapter:(MPRewardedVideoAdapter *)adapter
{
    MPLogAdEvent(MPLogEvent.adWillPresentModal, self.adUnitId);
    [self.delegate rewardedVideoDidReceiveTapEventForAdManager:self];
}

- (void)rewardedVideoDidReceiveImpressionEventForAdapter:(MPRewardedVideoAdapter *)adapter {
    [self.delegate rewardedVideoAdManager:self didReceiveImpressionEventWithImpressionData:self.configuration.impressionData];
}

- (void)rewardedVideoWillLeaveApplicationForAdapter:(MPRewardedVideoAdapter *)adapter
{
    MPLogAdEvent(MPLogEvent.adWillLeaveApplication, self.adUnitId);
    [self.delegate rewardedVideoWillLeaveApplicationForAdManager:self];
}

- (void)rewardedVideoShouldRewardUserForAdapter:(MPRewardedVideoAdapter *)adapter reward:(MPRewardedVideoReward *)reward
{
    MPLogAdEvent([MPLogEvent adShouldRewardUserWithReward:reward], self.adUnitId);
    [self.delegate rewardedVideoShouldRewardUserForAdManager:self reward:reward];
}

- (NSString *)rewardedVideoAdUnitId
{
    return self.adUnitId;
}

- (NSString *)rewardedVideoCustomerId
{
    return self.customerId;
}

@end

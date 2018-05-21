//
//  MPRewardedVideoAdManager.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRewardedVideoAdManager.h"

#import "MPAdServerCommunicator.h"
#import "MPAdServerURLBuilder.h"
#import "MPRewardedVideoAdapter.h"
#import "MPInstanceProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPRewardedVideoError.h"
#import "MPLogging.h"
#import "MoPub.h"

@interface MPRewardedVideoAdManager () <MPAdServerCommunicatorDelegate, MPRewardedVideoAdapterDelegate>

@property (nonatomic, strong) MPRewardedVideoAdapter *adapter;
@property (nonatomic, strong) MPAdServerCommunicator *communicator;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, assign) BOOL loading;
@property (nonatomic, assign) BOOL playedAd;
@property (nonatomic, assign) BOOL ready;

@end

@implementation MPRewardedVideoAdManager

- (instancetype)initWithAdUnitID:(NSString *)adUnitID delegate:(id<MPRewardedVideoAdManagerDelegate>)delegate
{
    if (self = [super init]) {
        _adUnitID = [adUnitID copy];
        _communicator = [[MPAdServerCommunicator alloc] initWithDelegate:self];
        _delegate = delegate;
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

- (void)loadRewardedVideoAdWithKeywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location customerId:(NSString *)customerId
{
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
        [self loadAdWithURL:[MPAdServerURLBuilder URLWithAdUnitID:self.adUnitID
                                                         keywords:keywords
                                                 userDataKeywords:userDataKeywords
                                                         location:location]];
    }
}

- (void)presentRewardedVideoAdFromViewController:(UIViewController *)viewController withReward:(MPRewardedVideoReward *)reward customData:(NSString *)customData
{
    // Don't allow the ad to be shown if it isn't ready.
    if (!self.ready) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdReady userInfo:@{ NSLocalizedDescriptionKey: @"Rewarded video ad view is not ready to be shown"}];

        // We don't want to remotely log this event -- it's simply for publisher troubleshooting -- so use NSLog
        // rather than MPLog.
        NSLog(@"%@", error.localizedDescription);
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
        MPLogWarn(@"Rewarded video manager is already loading an ad. "
                  @"Wait for previous load to finish.");
        return;
    }

    MPLogInfo(@"Rewarded video manager is loading ad with MoPub server URL: %@", URL);

    self.loading = YES;
    [self.communicator loadURL:URL];
}

#pragma mark - MPAdServerCommunicatorDelegate

- (void)communicatorDidReceiveAdConfigurations:(NSArray<MPAdConfiguration *> *)configurations
{
    self.configuration = configurations.firstObject;

    MPLogInfo(@"Rewarded video ad is fetching ad network type: %@", self.configuration.networkType);

    if (self.configuration.adUnitWarmingUp) {
        MPLogInfo(kMPWarmingUpErrorLogFormatWithAdUnitID, self.adUnitID);
        self.loading = NO;
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorAdUnitWarmingUp userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
        return;
    }

    if ([self.configuration.networkType isEqualToString:kAdTypeClear]) {
        MPLogInfo(kMPClearErrorLogFormatWithAdUnitID, self.adUnitID);
        self.loading = NO;
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
        return;
    }

    MPRewardedVideoAdapter *adapter = [[MPInstanceProvider sharedProvider] buildRewardedVideoAdapterWithDelegate:self];

    if (!adapter) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorUnknown userInfo:nil];
        [self rewardedVideoDidFailToLoadForAdapter:nil error:error];
        return;
    }

    self.adapter = adapter;
    [self.adapter getAdWithConfiguration:self.configuration];
}

- (void)communicatorDidFailWithError:(NSError *)error
{
    self.ready = NO;
    self.loading = NO;

    [self.delegate rewardedVideoDidFailToLoadForAdManager:self error:error];
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
    self.ready = YES;
    self.loading = NO;
    [self.delegate rewardedVideoDidLoadForAdManager:self];
}

- (void)rewardedVideoDidFailToLoadForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error
{
    self.ready = NO;
    self.loading = NO;
    [self loadAdWithURL:self.configuration.failoverURL];
}

- (void)rewardedVideoDidExpireForAdapter:(MPRewardedVideoAdapter *)adapter
{
    self.ready = NO;
    [self.delegate rewardedVideoDidExpireForAdManager:self];
}

- (void)rewardedVideoDidFailToPlayForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error
{
    [self.delegate rewardedVideoDidFailToPlayForAdManager:self error:error];
}

- (void)rewardedVideoWillAppearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    [self.delegate rewardedVideoWillAppearForAdManager:self];
}

- (void)rewardedVideoDidAppearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    [self.delegate rewardedVideoDidAppearForAdManager:self];
}

- (void)rewardedVideoWillDisappearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    [self.delegate rewardedVideoWillDisappearForAdManager:self];
}

- (void)rewardedVideoDidDisappearForAdapter:(MPRewardedVideoAdapter *)adapter
{
    self.ready = NO;
    self.playedAd = YES;
    [self.delegate rewardedVideoDidDisappearForAdManager:self];
}

- (void)rewardedVideoDidReceiveTapEventForAdapter:(MPRewardedVideoAdapter *)adapter
{
    [self.delegate rewardedVideoDidReceiveTapEventForAdManager:self];
}

- (void)rewardedVideoWillLeaveApplicationForAdapter:(MPRewardedVideoAdapter *)adapter
{
    [self.delegate rewardedVideoWillLeaveApplicationForAdManager:self];
}

- (void)rewardedVideoShouldRewardUserForAdapter:(MPRewardedVideoAdapter *)adapter reward:(MPRewardedVideoReward *)reward
{
    [self.delegate rewardedVideoShouldRewardUserForAdManager:self reward:reward];
}

- (NSString *)rewardedVideoAdUnitId
{
    return self.adUnitID;
}

- (NSString *)rewardedVideoCustomerId
{
    return self.customerId;
}

@end

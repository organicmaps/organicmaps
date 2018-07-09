//
//  MPRewardedVideo.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRewardedVideo.h"
#import "MPLogging.h"
#import "MPRewardedVideoAdManager.h"
#import "MPInstanceProvider.h"
#import "MPRewardedVideoError.h"
#import "MPRewardedVideoConnection.h"
#import "MPRewardedVideoCustomEvent.h"
#import "MPRewardedVideoCustomEvent+Caching.h"

static MPRewardedVideo *gSharedInstance = nil;

@interface MPRewardedVideo () <MPRewardedVideoAdManagerDelegate, MPRewardedVideoConnectionDelegate>

@property (nonatomic, strong) NSMutableDictionary *rewardedVideoAdManagers;
@property (nonatomic) NSMutableArray *rewardedVideoConnections;
@property (nonatomic, strong) NSMapTable<NSString *, id<MPRewardedVideoDelegate>> * delegateTable;

+ (MPRewardedVideo *)sharedInstance;

@end

@implementation MPRewardedVideo

- (instancetype)init
{
    if (self = [super init]) {
        _rewardedVideoAdManagers = [[NSMutableDictionary alloc] init];
        _rewardedVideoConnections = [NSMutableArray new];

        // Keys (ad unit ID) are strong, values (delegates) are weak.
        _delegateTable = [NSMapTable strongToWeakObjectsMapTable];
    }

    return self;
}

+ (void)setDelegate:(id<MPRewardedVideoDelegate>)delegate forAdUnitId:(NSString *)adUnitId
{
    if (adUnitId == nil) {
        return;
    }

    [[[self class] sharedInstance].delegateTable setObject:delegate forKey:adUnitId];
}

+ (void)removeDelegate:(id<MPRewardedVideoDelegate>)delegate
{
    if (delegate == nil) {
        return;
    }

    NSMapTable * mapTable = [[self class] sharedInstance].delegateTable;

    // Find all keys that contain the delegate
    NSMutableArray<NSString *> * keys = [NSMutableArray array];
    for (NSString * key in mapTable) {
        if ([mapTable objectForKey:key] == delegate) {
            [keys addObject:key];
        }
    }

    // Remove all of the found keys
    for (NSString * key in keys) {
        [mapTable removeObjectForKey:key];
    }
}

+ (void)removeDelegateForAdUnitId:(NSString *)adUnitId
{
    if (adUnitId == nil) {
        return;
    }

    [[[self class] sharedInstance].delegateTable removeObjectForKey:adUnitId];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID withMediationSettings:(NSArray *)mediationSettings
{
    [MPRewardedVideo loadRewardedVideoAdWithAdUnitID:adUnitID keywords:nil userDataKeywords:nil location:nil mediationSettings:mediationSettings];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location mediationSettings:(NSArray *)mediationSettings
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords location:location customerId:nil mediationSettings:mediationSettings];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location customerId:(NSString *)customerId mediationSettings:(NSArray *)mediationSettings
{
    MPRewardedVideo *sharedInstance = [[self class] sharedInstance];

    if (![adUnitID length]) {
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorInvalidAdUnitID userInfo:nil];
        id<MPRewardedVideoDelegate> delegate = [sharedInstance.delegateTable objectForKey:adUnitID];
        [delegate rewardedVideoAdDidFailToLoadForAdUnitID:adUnitID error:error];
        return;
    }

    MPRewardedVideoAdManager *adManager = sharedInstance.rewardedVideoAdManagers[adUnitID];

    if (!adManager) {
        adManager = [[MPInstanceProvider sharedProvider] buildRewardedVideoAdManagerWithAdUnitID:adUnitID delegate:sharedInstance];
        sharedInstance.rewardedVideoAdManagers[adUnitID] = adManager;
    }

    adManager.mediationSettings = mediationSettings;

    [adManager loadRewardedVideoAdWithKeywords:keywords userDataKeywords:userDataKeywords location:location customerId:customerId];
}

+ (BOOL)hasAdAvailableForAdUnitID:(NSString *)adUnitID
{
    MPRewardedVideo *sharedInstance = [[self class] sharedInstance];
    MPRewardedVideoAdManager *adManager = sharedInstance.rewardedVideoAdManagers[adUnitID];

    return [adManager hasAdAvailable];
}

+ (NSArray *)availableRewardsForAdUnitID:(NSString *)adUnitID
{
    MPRewardedVideo *sharedInstance = [[self class] sharedInstance];
    MPRewardedVideoAdManager *adManager = sharedInstance.rewardedVideoAdManagers[adUnitID];

    return adManager.availableRewards;
}

+ (MPRewardedVideoReward *)selectedRewardForAdUnitID:(NSString *)adUnitID
{
    MPRewardedVideo *sharedInstance = [[self class] sharedInstance];
    MPRewardedVideoAdManager *adManager = sharedInstance.rewardedVideoAdManagers[adUnitID];

    return adManager.selectedReward;
}

+ (void)presentRewardedVideoAdForAdUnitID:(NSString *)adUnitID fromViewController:(UIViewController *)viewController withReward:(MPRewardedVideoReward *)reward customData:(NSString *)customData
{
    MPRewardedVideo *sharedInstance = [[self class] sharedInstance];
    MPRewardedVideoAdManager *adManager = sharedInstance.rewardedVideoAdManagers[adUnitID];

    if (!adManager) {
        MPLogWarn(@"The rewarded video could not be shown: "
                  @"no ads have been loaded for adUnitID: %@", adUnitID);

        return;
    }

    if (!viewController) {
        MPLogWarn(@"The rewarded video could not be shown: "
                  @"a nil view controller was passed to -presentRewardedVideoAdForAdUnitID:fromViewController:.");

        return;
    }

    if (![viewController.view.window isKeyWindow]) {
        MPLogWarn(@"Attempting to present a rewarded video ad in non-key window. The ad may not render properly.");
    }

    [adManager presentRewardedVideoAdFromViewController:viewController withReward:reward customData:customData];
}

+ (void)presentRewardedVideoAdForAdUnitID:(NSString *)adUnitID fromViewController:(UIViewController *)viewController withReward:(MPRewardedVideoReward *)reward
{
    [MPRewardedVideo presentRewardedVideoAdForAdUnitID:adUnitID fromViewController:viewController withReward:reward customData:nil];
}

#pragma mark - Private

+ (MPRewardedVideo *)sharedInstance
{
    static dispatch_once_t once;

    dispatch_once(&once, ^{
        gSharedInstance = [[self alloc] init];
    });

    return gSharedInstance;
}

#pragma mark - MPRewardedVideoAdManagerDelegate

- (void)rewardedVideoDidLoadForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidLoadForAdUnitID:)]) {
        [delegate rewardedVideoAdDidLoadForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoDidFailToLoadForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidFailToLoadForAdUnitID:error:)]) {
        [delegate rewardedVideoAdDidFailToLoadForAdUnitID:manager.adUnitID error:error];
    }
}

- (void)rewardedVideoDidExpireForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidExpireForAdUnitID:)]) {
        [delegate rewardedVideoAdDidExpireForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoDidFailToPlayForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidFailToPlayForAdUnitID:error:)]) {
        [delegate rewardedVideoAdDidFailToPlayForAdUnitID:manager.adUnitID error:error];
    }
}

- (void)rewardedVideoWillAppearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillAppearForAdUnitID:)]) {
        [delegate rewardedVideoAdWillAppearForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoDidAppearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidAppearForAdUnitID:)]) {
        [delegate rewardedVideoAdDidAppearForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoWillDisappearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillDisappearForAdUnitID:)]) {
        [delegate rewardedVideoAdWillDisappearForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoDidDisappearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidDisappearForAdUnitID:)]) {
        [delegate rewardedVideoAdDidDisappearForAdUnitID:manager.adUnitID];
    }

    // Since multiple ad units may be attached to the same network, we should notify the custom events (which should then notify the application)
    // that their ads may not be available anymore since another ad unit might have "played" their ad. We go through and notify all ad managers
    // that are of the type of ad that is playing now.
    Class customEventClass = manager.customEventClass;

    for (id key in self.rewardedVideoAdManagers) {
        MPRewardedVideoAdManager *adManager = self.rewardedVideoAdManagers[key];

        if (adManager != manager && adManager.customEventClass == customEventClass) {
            [adManager handleAdPlayedForCustomEventNetwork];
        }
    }
}

- (void)rewardedVideoDidReceiveTapEventForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidReceiveTapEventForAdUnitID:)]) {
        [delegate rewardedVideoAdDidReceiveTapEventForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoWillLeaveApplicationForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillLeaveApplicationForAdUnitID:)]) {
        [delegate rewardedVideoAdWillLeaveApplicationForAdUnitID:manager.adUnitID];
    }
}

- (void)rewardedVideoShouldRewardUserForAdManager:(MPRewardedVideoAdManager *)manager reward:(MPRewardedVideoReward *)reward
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitID];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdShouldRewardForAdUnitID:reward:)]) {
        [delegate rewardedVideoAdShouldRewardForAdUnitID:manager.adUnitID reward:reward];
    }
}

#pragma mark - rewarded video server to server callback

- (void)startRewardedVideoConnectionWithUrl:(NSURL *)url
{
    MPRewardedVideoConnection *connection = [[MPRewardedVideoConnection alloc] initWithUrl:url delegate:self];
    [self.rewardedVideoConnections addObject:connection];
    [connection sendRewardedVideoCompletionRequest];
}

#pragma mark - MPRewardedVideoConnectionDelegate

- (void)rewardedVideoConnectionCompleted:(MPRewardedVideoConnection *)connection url:(NSURL *)url
{
    [self.rewardedVideoConnections removeObject:connection];
}

@end

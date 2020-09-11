//
//  MPRewardedVideo.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideo.h"
#import "MPAdTargeting.h"
#import "MPGlobal.h"
#import "MPImpressionTrackedNotification.h"
#import "MPLogging.h"
#import "MPRewardedVideoAdManager.h"
#import "MPRewardedVideoError.h"
#import "MPRewardedVideoConnection.h"
#import "MPRewardedVideoCustomEvent.h"
#import "MoPub+Utility.h"

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
    [MPRewardedVideo loadRewardedVideoAdWithAdUnitID:adUnitID keywords:nil userDataKeywords:nil customerId:nil mediationSettings:mediationSettings localExtras:nil];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location mediationSettings:(NSArray *)mediationSettings
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords customerId:nil mediationSettings:mediationSettings];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords mediationSettings:(NSArray *)mediationSettings
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords customerId:nil mediationSettings:mediationSettings];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location customerId:(NSString *)customerId mediationSettings:(NSArray *)mediationSettings
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords customerId:customerId mediationSettings:mediationSettings localExtras:nil];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords customerId:(NSString *)customerId mediationSettings:(NSArray *)mediationSettings
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords customerId:customerId mediationSettings:mediationSettings localExtras:nil];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords location:(CLLocation *)location customerId:(NSString *)customerId mediationSettings:(NSArray *)mediationSettings localExtras:(NSDictionary *)localExtras
{
    [self loadRewardedVideoAdWithAdUnitID:adUnitID keywords:keywords userDataKeywords:userDataKeywords customerId:customerId mediationSettings:mediationSettings localExtras:localExtras];
}

+ (void)loadRewardedVideoAdWithAdUnitID:(NSString *)adUnitID keywords:(NSString *)keywords userDataKeywords:(NSString *)userDataKeywords customerId:(NSString *)customerId mediationSettings:(NSArray *)mediationSettings localExtras:(NSDictionary *)localExtras
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
        adManager = [[MPRewardedVideoAdManager alloc] initWithAdUnitID:adUnitID delegate:sharedInstance];
        sharedInstance.rewardedVideoAdManagers[adUnitID] = adManager;
    }

    adManager.mediationSettings = mediationSettings;

    // Ad targeting options
    MPAdTargeting * targeting = [MPAdTargeting targetingWithCreativeSafeSize:MPApplicationFrame(YES).size];
    targeting.keywords = keywords;
    targeting.localExtras = localExtras;
    targeting.userDataKeywords = userDataKeywords;

    [adManager loadRewardedVideoAdWithCustomerId:customerId targeting:targeting];
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
        MPLogInfo(@"The rewarded video could not be shown: "
                  @"no ads have been loaded for adUnitID: %@", adUnitID);

        return;
    }

    if (!viewController) {
        MPLogInfo(@"The rewarded video could not be shown: "
                  @"a nil view controller was passed to -presentRewardedVideoAdForAdUnitID:fromViewController:.");

        return;
    }

    if (![viewController.view.window isKeyWindow]) {
        MPLogInfo(@"Attempting to present a rewarded video ad in non-key window. The ad may not render properly.");
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
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidLoadForAdUnitID:)]) {
        [delegate rewardedVideoAdDidLoadForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoDidFailToLoadForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidFailToLoadForAdUnitID:error:)]) {
        [delegate rewardedVideoAdDidFailToLoadForAdUnitID:manager.adUnitId error:error];
    }
}

- (void)rewardedVideoDidExpireForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidExpireForAdUnitID:)]) {
        [delegate rewardedVideoAdDidExpireForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoDidFailToPlayForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidFailToPlayForAdUnitID:error:)]) {
        [delegate rewardedVideoAdDidFailToPlayForAdUnitID:manager.adUnitId error:error];
    }
}

- (void)rewardedVideoWillAppearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillAppearForAdUnitID:)]) {
        [delegate rewardedVideoAdWillAppearForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoDidAppearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidAppearForAdUnitID:)]) {
        [delegate rewardedVideoAdDidAppearForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoWillDisappearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillDisappearForAdUnitID:)]) {
        [delegate rewardedVideoAdWillDisappearForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoDidDisappearForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidDisappearForAdUnitID:)]) {
        [delegate rewardedVideoAdDidDisappearForAdUnitID:manager.adUnitId];
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
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdDidReceiveTapEventForAdUnitID:)]) {
        [delegate rewardedVideoAdDidReceiveTapEventForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoAdManager:(MPRewardedVideoAdManager *)manager didReceiveImpressionEventWithImpressionData:(MPImpressionData *)impressionData
{
    [MoPub sendImpressionNotificationFromAd:nil
                                   adUnitID:manager.adUnitId
                             impressionData:impressionData];

    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(didTrackImpressionWithAdUnitID:impressionData:)]) {
        [delegate didTrackImpressionWithAdUnitID:manager.adUnitId impressionData:impressionData];
    }
}

- (void)rewardedVideoWillLeaveApplicationForAdManager:(MPRewardedVideoAdManager *)manager
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdWillLeaveApplicationForAdUnitID:)]) {
        [delegate rewardedVideoAdWillLeaveApplicationForAdUnitID:manager.adUnitId];
    }
}

- (void)rewardedVideoShouldRewardUserForAdManager:(MPRewardedVideoAdManager *)manager reward:(MPRewardedVideoReward *)reward
{
    id<MPRewardedVideoDelegate> delegate = [self.delegateTable objectForKey:manager.adUnitId];
    if ([delegate respondsToSelector:@selector(rewardedVideoAdShouldRewardForAdUnitID:reward:)]) {
        [delegate rewardedVideoAdShouldRewardForAdUnitID:manager.adUnitId reward:reward];
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

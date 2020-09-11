//
//  MPRewardedVideoAdManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPAdTargeting.h"
#import "MPImpressionData.h"

@class MPRewardedVideoReward;
@protocol MPRewardedVideoAdManagerDelegate;

/**
 * `MPRewardedVideoAdManager` represents a rewarded video for a single ad unit ID. This is the object that
 * `MPRewardedVideo` uses to load and present the ad.
 */
@interface MPRewardedVideoAdManager : NSObject

@property (nonatomic, weak) id<MPRewardedVideoAdManagerDelegate> delegate;
@property (nonatomic, readonly) NSString *adUnitId;
@property (nonatomic, strong) NSArray *mediationSettings;
@property (nonatomic, copy) NSString *customerId;
@property (nonatomic, strong) MPAdTargeting *targeting;

/**
 * An array of rewards that are available for the rewarded ad that can be selected when presenting the ad.
 */
@property (nonatomic, readonly) NSArray *availableRewards;

/**
 * The currently selected reward that will be awarded to the user upon completion of the ad. By default,
 * this corresponds to the first reward in `availableRewards`.
 */
@property (nonatomic, readonly) MPRewardedVideoReward *selectedReward;

- (instancetype)initWithAdUnitID:(NSString *)adUnitID delegate:(id<MPRewardedVideoAdManagerDelegate>)delegate;

/**
 * Returns the custom event class type.
 */
- (Class)customEventClass;

/**
 * Loads a rewarded video ad with the ad manager's ad unit ID.
 *
 * @param customerId The user's id within the app.
 * @param targeting Optional ad targeting parameters.
 *
 * However, if an ad has been played for the last time a load was issued and load is called again, the method will request a new ad.
 */
- (void)loadRewardedVideoAdWithCustomerId:(NSString *)customerId targeting:(MPAdTargeting *)targeting;

/**
 * Tells the caller whether the underlying ad network currently has an ad available for presentation.
 */
- (BOOL)hasAdAvailable;

/**
 * Plays a rewarded video ad.
 *
 * @param viewController Presents the rewarded video ad from viewController.
 * @param reward A reward chosen from `availableRewards` to award the user upon completion.
 * This value should not be `nil`. If the reward that is passed in did not come from `availableRewards`,
 * this method will not present the rewarded ad and invoke `rewardedVideoDidFailToPlayForAdManager:error:`.
 * @param customData Optional custom data string to include in the server-to-server callback. If a server-to-server callback
 * is not used, or if the ad unit is configured for local rewarding, this value will not be persisted.
 */
- (void)presentRewardedVideoAdFromViewController:(UIViewController *)viewController withReward:(MPRewardedVideoReward *)reward customData:(NSString *)customData;

/**
 * This method is called when another ad unit has played a rewarded video from the same network this ad manager's custom event
 * represents.
 */
- (void)handleAdPlayedForCustomEventNetwork;

@end

@protocol MPRewardedVideoAdManagerDelegate <NSObject>

- (void)rewardedVideoDidLoadForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoDidFailToLoadForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error;
- (void)rewardedVideoDidExpireForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoDidFailToPlayForAdManager:(MPRewardedVideoAdManager *)manager error:(NSError *)error;
- (void)rewardedVideoWillAppearForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoDidAppearForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoWillDisappearForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoDidDisappearForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoDidReceiveTapEventForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoAdManager:(MPRewardedVideoAdManager *)manager didReceiveImpressionEventWithImpressionData:(MPImpressionData *)impressionData;
- (void)rewardedVideoWillLeaveApplicationForAdManager:(MPRewardedVideoAdManager *)manager;
- (void)rewardedVideoShouldRewardUserForAdManager:(MPRewardedVideoAdManager *)manager reward:(MPRewardedVideoReward *)reward;

@end

//
//  MPRewardedVideoAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@class MPAdConfiguration;
@class MPAdTargeting;
@class MPRewardedVideoReward;

@protocol MPRewardedVideoAdapterDelegate;
@protocol MPMediationSettingsProtocol;

/**
 * `MPRewardedVideoAdapter` directly communicates with the appropriate custom event to
 * load and show a rewarded video. It is also the class that handles impression
 * and click tracking. Finally, the class will report a failure to load an ad if the ad
 * takes too long to load.
 */
@interface MPRewardedVideoAdapter : NSObject <MPPrivateRewardedVideoCustomEventDelegate>

@property (nonatomic, weak) id<MPRewardedVideoAdapterDelegate> delegate;

- (instancetype)initWithDelegate:(id<MPRewardedVideoAdapterDelegate>)delegate;

/**
 * Called to retrieve an ad once we get a response from the server.
 *
 * @param configuration Contains the details about the ad we are loading.
 8 @param targeting Optional ad targeting details for the ad we are loading.
 */
- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting;

/**
 * Tells the caller whether the underlying ad network currently has an ad available for presentation.
 */
- (BOOL)hasAdAvailable;

/**
 * Plays a rewarded video ad.
 *
 * @param viewController Presents the rewarded video ad from viewController.
 * @param customData Optional custom data string to include in the server-to-server callback. If a server-to-server callback
 * is not used, or if the ad unit is configured for local rewarding, this value will not be persisted.
 */
- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController customData:(NSString *)customData;

/**
 * This method is called when another ad unit has played a rewarded video from the same network this adapter's custom event
 * represents.
 */
- (void)handleAdPlayedForCustomEventNetwork;

@end

@protocol MPRewardedVideoAdapterDelegate <NSObject>

- (id<MPMediationSettingsProtocol>)instanceMediationSettingsForClass:(Class)aClass;

- (void)rewardedVideoDidLoadForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidFailToLoadForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error;
- (void)rewardedVideoDidExpireForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidFailToPlayForAdapter:(MPRewardedVideoAdapter *)adapter error:(NSError *)error;
- (void)rewardedVideoWillAppearForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidAppearForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoWillDisappearForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidDisappearForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidReceiveTapEventForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoDidReceiveImpressionEventForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoWillLeaveApplicationForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoShouldRewardUserForAdapter:(MPRewardedVideoAdapter *)adapter reward:(MPRewardedVideoReward *)reward;

@optional
- (NSString *)rewardedVideoAdUnitId;
- (NSString *)rewardedVideoCustomerId;
- (MPAdConfiguration *)configuration;

@end

//
//  MPRewardedVideoAdapter.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@class MPAdConfiguration;
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
 */
- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration;

/**
 * Tells the caller whether the underlying ad network currently has an ad available for presentation.
 */
- (BOOL)hasAdAvailable;

/**
 * Plays a rewarded video ad.
 *
 * @param viewController Presents the rewarded video ad from viewController.
 */
- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController;

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
- (void)rewardedVideoWillLeaveApplicationForAdapter:(MPRewardedVideoAdapter *)adapter;
- (void)rewardedVideoShouldRewardUserForAdapter:(MPRewardedVideoAdapter *)adapter reward:(MPRewardedVideoReward *)reward;

@optional
- (NSString *)rewardedVideoAdUnitId;
- (NSString *)rewardedVideoCustomerId;
- (MPAdConfiguration *)configuration;

@end

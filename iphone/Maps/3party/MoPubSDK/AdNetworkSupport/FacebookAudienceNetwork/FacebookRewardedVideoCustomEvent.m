//
//  FacebookRewardedVideoCustomEvent.m
//
//  Created by Mopub on 4/12/17.
//
#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookRewardedVideoCustomEvent.h"

#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MoPub.h"
#import "MPRewardedVideoReward.h"
#import "MPRealTimeTimer.h"

//Timer to record the expiration interval
#define FB_ADS_EXPIRATION_INTERVAL  3600

@interface MPInstanceProvider (FacebookRewardedVideos)

- (FBRewardedVideoAd *)buildFBRewardedVideoAdWithPlacementID:(NSString *)placementID
                                                    delegate:(id<FBRewardedVideoAdDelegate>)delegate;

@end

@implementation MPInstanceProvider (FacebookRewardedVideos)

- (FBRewardedVideoAd *)buildFBRewardedVideoAdWithPlacementID:(NSString *)placementID
                                                    delegate:(id<FBRewardedVideoAdDelegate>)delegate
{
    FBRewardedVideoAd *rewardedVideoAd = [[FBRewardedVideoAd alloc] initWithPlacementID:placementID];
    rewardedVideoAd.delegate = delegate;
    return rewardedVideoAd;
}

@end


@interface FacebookRewardedVideoCustomEvent () <FBRewardedVideoAdDelegate>

@property (nonatomic, strong) FBRewardedVideoAd *fbRewardedVideoAd;
@property (nonatomic, strong) MPRealTimeTimer *expirationTimer;
@property (nonatomic, assign) BOOL hasTrackedImpression;

@end

@implementation FacebookRewardedVideoCustomEvent

@synthesize hasTrackedImpression = _hasTrackedImpression;

- (void)initializeSdkWithParameters:(NSDictionary *)parameters {
    // No SDK initialization method provided.
}

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info {
    [self requestRewardedVideoWithCustomEventInfo:info adMarkup:nil];
}

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup {
    if (![info objectForKey:@"placement_id"]) {
        MPLogError(@"Placement ID is required for Facebook Rewarded Video ad");
        [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:nil];
        return;
    }
    
    self.fbRewardedVideoAd =
    [[MPInstanceProvider sharedProvider] buildFBRewardedVideoAdWithPlacementID: [info objectForKey:@"placement_id"] delegate:self];
    
    [FBAdSettings setMediationService:[NSString stringWithFormat:@"MOPUB_%@", MP_SDK_VERSION]];
    // Load the advanced bid payload.
    if (adMarkup != nil) {
        MPLogInfo(@"Loading Facebook rewarded video ad markup");
        [self.fbRewardedVideoAd loadAdWithBidPayload:adMarkup];
    }
    // Request a rewarded video ad.
    else {
        MPLogInfo(@"Requesting Facebook rewarded video ad");
        [self.fbRewardedVideoAd loadAd];
    }
}

//Verify that the rewarded video is precached
- (BOOL)hasAdAvailable
{
    return (self.fbRewardedVideoAd != nil && self.fbRewardedVideoAd.isAdValid);
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController
{
    if(![self hasAdAvailable])
    {
        MPLogError(@"Facebook rewarded video ad was not available");
        NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain code:MPRewardedVideoAdErrorNoAdsAvailable userInfo:nil];
        [self.delegate rewardedVideoDidFailToPlayForCustomEvent:self error:error];
    }
    else
    {
        MPLogInfo(@"Facebook rewarded video ad will be presented");
        [self.delegate rewardedVideoWillAppearForCustomEvent:self];
        [self.fbRewardedVideoAd showAdFromRootViewController:viewController];
        MPLogInfo(@"Facebook rewarded video ad was presented");
        [self.delegate rewardedVideoDidAppearForCustomEvent:self];
    }
}

-(void)dealloc{
    _fbRewardedVideoAd.delegate = nil;
}

#pragma mark FBRewardedVideoAdDelegate methods

/*!
 @method
 
 @abstract
 Sent after an ad has been clicked by the person.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdDidClick:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video ad was clicked");
    [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self ];
}

/*!
 @method
 
 @abstract
 Sent when an ad has been successfully loaded.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdDidLoad:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video ad was loaded. Can present now.");
    [self.delegate rewardedVideoDidLoadAdForCustomEvent:self ];
    
    // introduce timer for 1 hour as per caching logic introduced by FB
    __weak __typeof__(self) weakSelf = self;
    self.expirationTimer = [[MPRealTimeTimer alloc] initWithInterval:FB_ADS_EXPIRATION_INTERVAL block:^(MPRealTimeTimer *timer){
        __strong __typeof__(weakSelf) strongSelf = weakSelf;
        if (strongSelf && !strongSelf.hasTrackedImpression) {
            [self.delegate rewardedVideoDidExpireForCustomEvent:self];
            MPLogInfo(@"Facebook Rewarded Video ad expired as per the audience network's caching policy");
            self.fbRewardedVideoAd = nil;
        }
        [strongSelf.expirationTimer invalidate];
    }];
    [self.expirationTimer scheduleNow];
}

/*!
 @method
 
 @abstract
 Sent after an FBRewardedVideoAd object has been dismissed from the screen, returning control
 to your application.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdDidClose:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video ad is dismissed.");
    [self.delegate rewardedVideoDidDisappearForCustomEvent:self];
}

/*!
 @method
 
 @abstract
 Sent immediately before an FBRewardedVideoAd object will be dismissed from the screen.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdWillClose:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video ad will be dismissed.");
    [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
}

/*!
 @method
 
 @abstract
 Sent after an FBRewardedVideoAd fails to load the ad.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 @param error An error object containing details of the error.
 */
- (void)rewardedVideoAd:(FBRewardedVideoAd *)rewardedVideoAd didFailWithError:(NSError *)error
{
    MPLogInfo(@"Facebook rewarded video ad failed to load with error: %@", error.localizedDescription);
    [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:error];
}

/*!
 @method
 
 @abstract
 Sent after the FBRewardedVideoAd object has finished playing the video successfully.
 Reward the user on this callback.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdVideoComplete:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video ad has finished playing successfully");
    
    // Passing the reward type and amount as unspecified. Set the reward value in mopub UI.
    [self.delegate rewardedVideoShouldRewardUserForCustomEvent:self reward:[[MPRewardedVideoReward alloc] initWithCurrencyAmount:@(kMPRewardedVideoRewardCurrencyAmountUnspecified)]];
}

/*!
 @method
 
 @abstract
 Sent immediately before the impression of an FBRewardedVideoAd object will be logged.
 
 @param rewardedVideoAd An FBRewardedVideoAd object sending the message.
 */
- (void)rewardedVideoAdWillLogImpression:(FBRewardedVideoAd *)rewardedVideoAd
{
    MPLogInfo(@"Facebook rewarded video has started playing and hence logging impression");
    //set the tracker to true when the ad is shown on the screen. So that the timer is invalidated.
    _hasTrackedImpression = true;
    [self.expirationTimer invalidate];
}

@end



#import "MPGoogleAdMobRewardedVideoCustomEvent.h"

#import <GoogleMobileAds/GoogleMobileAds.h>
#import "MPLogging.h"
#import "MPRewardedVideoError.h"
#import "MPRewardedVideoReward.h"
#import "MPRewardedVideoCustomEvent+Caching.h"

@interface MPGoogleAdMobRewardedVideoCustomEvent () <GADRewardBasedVideoAdDelegate>

@end

@implementation MPGoogleAdMobRewardedVideoCustomEvent

- (void)initializeSdkWithParameters:(NSDictionary *)parameters {
  NSString *applicationID = [parameters objectForKey:@"appid"];
  if (applicationID) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      [GADMobileAds configureWithApplicationID:applicationID];
    });
  }
}

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info {
  [self initializeSdkWithParameters:info];

  // Cache the network initialization parameters
  [self setCachedInitializationParameters:info];

  NSString *adUnitID = [info objectForKey:@"adunit"];
  if (!adUnitID) {
    NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain
                                         code:MPRewardedVideoAdErrorInvalidAdUnitID
                                     userInfo:@{NSLocalizedDescriptionKey:
                                                @"Ad Unit ID cannot be nil."}];
    [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:error];
    return;
  }

  GADRequest *request = [GADRequest request];
  request.requestAgent = @"MoPub";

  [GADRewardBasedVideoAd sharedInstance].delegate = self;
  [[GADRewardBasedVideoAd sharedInstance] loadRequest:request withAdUnitID:adUnitID];
}

- (BOOL)hasAdAvailable {
  return [GADRewardBasedVideoAd sharedInstance].isReady;
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController {
  if ([self hasAdAvailable]) {
    [[GADRewardBasedVideoAd sharedInstance] presentFromRootViewController:viewController];
  }
  else {
    // We will send the error if the reward-based video ad has already been presented.
    NSError *error = [NSError errorWithDomain:MoPubRewardedVideoAdsSDKDomain
                                         code:MPRewardedVideoAdErrorAdAlreadyPlayed
                                     userInfo:@{NSLocalizedDescriptionKey:
                                                @"Reward-based video ad has already been shown."}];
    [self.delegate rewardedVideoDidFailToPlayForCustomEvent:self error:error];
  }
}

- (BOOL)enableAutomaticImpressionAndClickTracking {
  return NO;
}

// MoPub's API includes this method because it's technically possible for two MoPub custom events or
// adapters to wrap the same SDK and therefore both claim ownership of the same cached ad. The
// method will be called if 1) this custom event has already invoked
// rewardedVideoDidLoadAdForCustomEvent: on the delegate, and 2) some other custom event plays a
// rewarded video ad. It's a way of forcing this custom event to double-check that its ad is
// definitely still available and is not the one that just played. If the ad is still available, no
// action is necessary. If it's not, this custom event should call
// rewardedVideoDidExpireForCustomEvent: to let the MoPub SDK know that it's no longer ready to play
// and needs to load another ad. That event will be passed on to the publisher app, which can then
// trigger another load.
- (void)handleAdPlayedForCustomEventNetwork {
  if (![self hasAdAvailable]) {
    // Sending rewardedVideoDidExpireForCustomEvent: callback because the reward-based video ad will
    // not be available once its been presented.
    [self.delegate rewardedVideoDidExpireForCustomEvent:self];
  }
}

#pragma mark - GADRewardBasedVideoAdDelegate methods

- (void)rewardBasedVideoAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd
   didRewardUserWithReward:(GADAdReward *)reward {
  MPRewardedVideoReward *moPubReward = [[MPRewardedVideoReward alloc]
                                      initWithCurrencyType:reward.type
                                                    amount:reward.amount];
  [self.delegate rewardedVideoShouldRewardUserForCustomEvent:self reward:moPubReward];
}

- (void)rewardBasedVideoAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd
    didFailToLoadWithError:(NSError *)error {
  [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:error];
}

- (void)rewardBasedVideoAdDidReceiveAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
  [self.delegate rewardedVideoDidLoadAdForCustomEvent:self];
}

- (void)rewardBasedVideoAdDidOpen:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
  [self.delegate rewardedVideoWillAppearForCustomEvent:self];
  [self.delegate rewardedVideoDidAppearForCustomEvent:self];
  // Recording an impression after the reward-based video ad appears on the screen.
  [self.delegate trackImpression];
}

- (void)rewardBasedVideoAdDidStartPlaying:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
  // MoPub rewarded video custom event doesn't have a callback when the video starts playing.
  MPLogInfo(@"Google AdMob reward-based video ad started playing.");
}

- (void)rewardBasedVideoAdDidClose:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
  [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
  [self.delegate rewardedVideoDidDisappearForCustomEvent:self];
}

- (void)rewardBasedVideoAdWillLeaveApplication:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
  // Recording a click because the rewardBasedVideoAdWillLeaveApplication: is invoked when a click
  // on the reward-based video ad happens.
  [self.delegate trackClick];
  [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
  [self.delegate rewardedVideoWillLeaveApplicationForCustomEvent:self];
}

@end

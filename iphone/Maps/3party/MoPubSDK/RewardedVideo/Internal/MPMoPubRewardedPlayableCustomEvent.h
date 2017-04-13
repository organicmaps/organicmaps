//
//  MPMoPubRewardedPlayableCustomEvent.h
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent.h"
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@interface MPMoPubRewardedPlayableCustomEvent : MPRewardedVideoCustomEvent
@property (nonatomic, readonly) NSTimeInterval countdownDuration;

@property (nonatomic, weak) id<MPPrivateRewardedVideoCustomEventDelegate> delegate;
@end

//
//  MPMoPubRewardedVideoCustomEvent.h
//  MoPubSDK

//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent.h"
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@interface MPMoPubRewardedVideoCustomEvent : MPRewardedVideoCustomEvent

@property (nonatomic, weak) id<MPPrivateRewardedVideoCustomEventDelegate> delegate;

@end

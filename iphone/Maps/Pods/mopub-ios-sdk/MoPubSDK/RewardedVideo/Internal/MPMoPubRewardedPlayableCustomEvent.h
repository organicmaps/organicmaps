//
//  MPMoPubRewardedPlayableCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideoCustomEvent.h"
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@interface MPMoPubRewardedPlayableCustomEvent : MPRewardedVideoCustomEvent

@property (nonatomic, weak) id<MPPrivateRewardedVideoCustomEventDelegate> delegate;
@end

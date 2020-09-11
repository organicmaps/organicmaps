//
//  MPMoPubRewardedVideoCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent.h"
#import "MPPrivateRewardedVideoCustomEventDelegate.h"

@interface MPMoPubRewardedVideoCustomEvent : MPRewardedVideoCustomEvent

@property (nonatomic, weak) id<MPPrivateRewardedVideoCustomEventDelegate> delegate;

@end

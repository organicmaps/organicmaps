//
//  MPPrivateRewardedVideoCustomEventDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideoCustomEvent.h"

@class MPAdConfiguration;
@class CLLocation;

@protocol MPPrivateRewardedVideoCustomEventDelegate <MPRewardedVideoCustomEventDelegate>

- (NSString *)adUnitId;
- (MPAdConfiguration *)configuration;

@end

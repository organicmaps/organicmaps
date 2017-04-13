//
//  MPPrivateRewardedVideoCustomEventDelegate.h
//  MoPub
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent.h"

@class MPAdConfiguration;
@class CLLocation;

@protocol MPPrivateRewardedVideoCustomEventDelegate <MPRewardedVideoCustomEventDelegate>

- (NSString *)adUnitId;
- (MPAdConfiguration *)configuration;

@end

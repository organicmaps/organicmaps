//
//  MPRewardedVideoCustomEvent+Caching.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent+Caching.h"
#import "MPLogging.h"
#import "MPMediationManager.h"

@implementation MPRewardedVideoCustomEvent (Caching)

- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params {
    [MPMediationManager.sharedManager setCachedInitializationParameters:params forNetwork:[self class]];
}

- (NSDictionary * _Nullable)cachedInitializationParameters {
    return [MPMediationManager.sharedManager cachedInitializationParametersForNetwork:[self class]];
}

@end

//
//  MPMoPubConfiguration.m
//  MoPubSampleApp
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPMoPubConfiguration.h"

@implementation MPMoPubConfiguration

- (instancetype)initWithAdUnitIdForAppInitialization:(NSString * _Nonnull)adUnitId {
    if (self = [super init]) {
        _adUnitIdForAppInitialization = adUnitId;
        _advancedBidders = nil;
        _globalMediationSettings = nil;
        _mediatedNetworks = nil;
    }

    return self;
}

@end

//
//  MPMoPubConfiguration.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMoPubConfiguration.h"
#import "MPAdapterConfiguration.h"
#import "MPLogging.h"

@implementation MPMoPubConfiguration

- (instancetype)initWithAdUnitIdForAppInitialization:(NSString * _Nonnull)adUnitId {
    if (self = [super init]) {
        _additionalNetworks = nil;
        _adUnitIdForAppInitialization = adUnitId;
        _allowLegitimateInterest = NO;
        _globalMediationSettings = nil;
        _loggingLevel = MPBLogLevelNone;
        _mediatedNetworkConfigurations = nil;
        _moPubRequestOptions = nil;
    }

    return self;
}

- (void)setNetworkConfiguration:(NSDictionary<NSString *, id> *)configuration
            forMediationAdapter:(NSString *)adapterClassName {
    // Invalid adapter class name
    if (adapterClassName == nil) {
        return;
    }

    // Adapter class name does not exist in the runtime.
    Class adapterClass = NSClassFromString(adapterClassName);
    if (adapterClass == Nil) {
        MPLogDebug(@"%@ is not a valid class name.", adapterClassName);
        return;
    }

    // Adapter class name does not conform to `MPAdapterConfiguration`
    if (![adapterClass conformsToProtocol:@protocol(MPAdapterConfiguration)]) {
        MPLogDebug(@"%@ does not conform to MPAdapterConfiguration protocol.", adapterClassName);
        return;
    }

    // Lazy initialization
    if (self.mediatedNetworkConfigurations == nil) {
        self.mediatedNetworkConfigurations = [NSMutableDictionary dictionaryWithCapacity:1];
    }

    // Add the entry
    self.mediatedNetworkConfigurations[adapterClassName] = configuration;
}

- (void)setMoPubRequestOptions:(NSDictionary<NSString *, NSString *> * _Nullable)options
           forMediationAdapter:(NSString *)adapterClassName {
    // Invalid adapter class name
    if (adapterClassName == nil) {
        return;
    }

    // Adapter class name does not exist in the runtime.
    Class adapterClass = NSClassFromString(adapterClassName);
    if (adapterClass == Nil) {
        MPLogDebug(@"%@ is not a valid class name.", adapterClassName);
        return;
    }

    // Adapter class name does not conform to `MPAdapterConfiguration`
    if (![adapterClass conformsToProtocol:@protocol(MPAdapterConfiguration)]) {
        MPLogDebug(@"%@ does not conform to MPAdapterConfiguration protocol.", adapterClassName);
        return;
    }

    // Lazy initialization
    if (self.moPubRequestOptions == nil) {
        self.moPubRequestOptions = [NSMutableDictionary dictionaryWithCapacity:1];
    }

    // Add the entry
    self.moPubRequestOptions[adapterClassName] = options;
}

@end

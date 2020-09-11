//
//  MPBaseAdapterConfiguration.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseAdapterConfiguration.h"
#import "MPMediationManager.h"

@interface MPBaseAdapterConfiguration()
@property (nonatomic, readonly) NSMutableDictionary<NSString *, NSString *> * internalMopubRequestOptions;
@end

@implementation MPBaseAdapterConfiguration
@dynamic adapterVersion;
@dynamic biddingToken;
@dynamic moPubNetworkName;
@dynamic networkSdkVersion;

#pragma mark - Initialization

- (instancetype)init {
    if (self = [super init]) {
        _internalMopubRequestOptions = [NSMutableDictionary dictionary];
    }

    return self;
}

#pragma mark - MPAdapterConfiguration Default Implementations

- (NSDictionary<NSString *, NSString *> *)moPubRequestOptions {
    return self.internalMopubRequestOptions;
}

- (void)initializeNetworkWithConfiguration:(NSDictionary<NSString *, id> * _Nullable)configuration
                                  complete:(void(^ _Nullable)(NSError * _Nullable))complete {
    if (complete != nil) {
        complete(nil);
    }
}

- (void)addMoPubRequestOptions:(NSDictionary<NSString *, NSString *> *)options {
    // No entries to add
    if (options == nil) {
        return;
    }

    [self.internalMopubRequestOptions addEntriesFromDictionary:options];
}

+ (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params {
    [MPMediationManager.sharedManager setCachedInitializationParameters:params forNetwork:self.class];
}

+ (NSDictionary * _Nullable)cachedInitializationParameters {
    return [MPMediationManager.sharedManager cachedInitializationParametersForNetwork:self.class];
}

@end

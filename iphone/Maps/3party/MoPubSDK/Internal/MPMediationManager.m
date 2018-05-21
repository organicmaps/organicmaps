//
//  MPMediationManager.m
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import "MPMediationManager.h"
#import "MPLogging.h"

/**
 Key of the @c NSUserDefaults entry for the network initialization cache.
 */
static NSString * const kNetworkSDKInitializationParametersKey = @"com.mopub.mopub-ios-sdk.network-init-info";

@implementation MPMediationManager

#pragma mark - Initialization

+ (instancetype)sharedManager {
    static MPMediationManager * sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[MPMediationManager alloc] init];
    });

    return sharedInstance;
}

#pragma mark - Cache

- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params forNetwork:(Class<MPMediationSdkInitializable> _Nonnull)networkClass {
    // Empty network names and parameters are invalid.
    NSString * network = NSStringFromClass(networkClass);
    if (network.length == 0 || params == nil) {
        return;
    }

    @synchronized (self) {
        NSMutableDictionary * cachedParameters = [[[NSUserDefaults standardUserDefaults] objectForKey:kNetworkSDKInitializationParametersKey] mutableCopy];
        if (cachedParameters == nil) {
            cachedParameters = [NSMutableDictionary dictionaryWithCapacity:1];
        }

        cachedParameters[network] = params;
        [[NSUserDefaults standardUserDefaults] setObject:cachedParameters forKey:kNetworkSDKInitializationParametersKey];
        [[NSUserDefaults standardUserDefaults] synchronize];

        MPLogInfo(@"Cached SDK initialization parameters for %@:\n%@", network, params);
    }
}

- (NSDictionary * _Nullable)cachedInitializationParametersForNetwork:(Class<MPMediationSdkInitializable>)networkClass {
    // Empty network names are invalid.
    NSString * network = NSStringFromClass(networkClass);
    if (network.length == 0) {
        return nil;
    }

    NSDictionary * cachedParameters = [[NSUserDefaults standardUserDefaults] objectForKey:kNetworkSDKInitializationParametersKey];
    if (cachedParameters == nil) {
        return nil;
    }

    return [cachedParameters objectForKey:network];
}

- (NSArray<Class<MPMediationSdkInitializable>> * _Nullable)allCachedNetworks {
    NSDictionary * cachedParameters = [[NSUserDefaults standardUserDefaults] objectForKey:kNetworkSDKInitializationParametersKey];
    NSArray * cacheKeys = [cachedParameters allKeys];
    if (cacheKeys == nil) {
        return nil;
    }

    // Convert the strings of class names into class types.
    NSMutableArray<Class<MPMediationSdkInitializable>> * cachedNetworks = [NSMutableArray array];
    [cacheKeys enumerateObjectsUsingBlock:^(NSString * key, NSUInteger idx, BOOL * _Nonnull stop) {
        Class c = NSClassFromString(key);
        if ([c conformsToProtocol:@protocol(MPMediationSdkInitializable)]) {
            [cachedNetworks addObject:c];
        }
    }];

    return cachedNetworks;
}

- (void)clearCache {
    @synchronized (self) {
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:kNetworkSDKInitializationParametersKey];
        [[NSUserDefaults standardUserDefaults] synchronize];

        MPLogInfo(@"Cleared cached SDK initialization parameters");
    }
}

#pragma mark - Mediation

- (void)initializeMediatedNetworks:(NSArray<Class<MPMediationSdkInitializable>> *)networks
                        completion:(void (^ _Nullable)(NSError * _Nullable error))completion {
    // Nothing to initialize
    if (networks.count == 0) {
        if (completion != nil) {
            completion(nil);
        }

        return;
    }

    // Network SDK initializations should occur on the main thread since
    // some of those SDKs require it.
    dispatch_async(dispatch_get_main_queue(), ^{
        for (Class<MPMediationSdkInitializable> mediationClass in networks) {
            id<MPMediationSdkInitializable> mediationNetwork = (id<MPMediationSdkInitializable>)[[mediationClass class] new];
            NSDictionary * cachedInitializationParams = [self cachedInitializationParametersForNetwork:mediationClass];

            // Only attempt initialization if they exist. Otherwise, we should wait for the
            // on-demand initialization to occur with the correct parameters.
            if (cachedInitializationParams != nil) {
                [mediationNetwork initializeSdkWithParameters:cachedInitializationParams];
                MPLogInfo(@"Loaded mediated network: %@", NSStringFromClass(mediationClass));
            }
        }

        if (completion != nil) {
            completion(nil);
        }
    });
}

@end

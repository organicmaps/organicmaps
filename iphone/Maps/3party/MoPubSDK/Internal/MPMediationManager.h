//
//  MPMediationManager.h
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPMediationSdkInitializable.h"

@interface MPMediationManager : NSObject

/**
 Singleton instance of the manager.
 */
+ (instancetype _Nonnull)sharedManager;

/**
 Initializes the inputted mediated network SDKs from the cache.
 @param networks Networks to initialize. If @c nil, nothing will be done.
 @param completion Optional completion block.
 */
- (void)initializeMediatedNetworks:(NSArray<Class<MPMediationSdkInitializable>> * _Nullable)networks
                        completion:(void (^ _Nullable)(NSError * _Nullable error))completion;

/**
 Sets the initialization parameters for a given network in the cache.
 @param params Initialization parameters sent to the @c MPSdkInitializable instance when initialized.
 @param networkClass Network class.
 */
- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params forNetwork:(Class<MPMediationSdkInitializable> _Nonnull)networkClass;

/**
 Retrieves the cached initialization parameters for a given network.
 @param networkClass Network class.
 @returns The cached parameters or @c nil.
 */
- (NSDictionary * _Nullable)cachedInitializationParametersForNetwork:(Class<MPMediationSdkInitializable> _Nonnull)networkClass;

/**
 Retrieves all of the currently cached networks.
 @return A list of all cached networks or @c nil.
 */
- (NSArray<Class<MPMediationSdkInitializable>> * _Nullable)allCachedNetworks;

/**
 Clears the cache.
 */
- (void)clearCache;

@end

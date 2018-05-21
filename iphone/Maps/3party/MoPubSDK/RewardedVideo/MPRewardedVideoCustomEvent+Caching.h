//
//  MPRewardedVideoCustomEvent+Caching.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPRewardedVideoCustomEvent.h"

/**
 * Provides caching support for network SDK initialization parameters.
 */
@interface MPRewardedVideoCustomEvent (Caching)

/**
 * Updates the initialization parameters for the current network.
 * @param params New set of initialization parameters. Nothing will be done if `nil` is passed in.
 */
- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params;

/**
 * Retrieves the initialization parameters for the current network (if any).
 * @return The cached initialization parameters for the network. This may be `nil` if not parameters were found.
 */
- (NSDictionary * _Nullable)cachedInitializationParameters;

@end

//
//  MPMediationManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPAdapterConfiguration.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Initialization completion block.
 */
typedef void(^MPMediationInitializationCompletionBlock)(NSError * _Nullable error, NSArray<id<MPAdapterConfiguration>> * _Nullable initializedAdapters);

/**
 Manages all mediated network adapters that interface with the MoPub SDK.
 */
@interface MPMediationManager : NSObject

/**
 Dictionary of all instantiated adapter information providers.
 */
@property (nonatomic, readonly) NSMutableDictionary<NSString *, id<MPAdapterConfiguration>> * adapters;

/**
 Optional JSON payload to include with every MoPub ad request using the @c kNetworkAdaptersKey metadata key.
 This value may be @c nil if there are no initialized adapter information providers in the runtime.
 */
@property (nonatomic, readonly, nullable) NSDictionary<NSString *, NSDictionary *> * adRequestPayload;

/**
 Singleton instance of the manager.
 */
+ (instancetype)sharedManager;

/**
 Initializes the specified adapter information providers and their underlying network SDKs.
 @param providers Optional additional adapter information providers to initialize along with the officially supported networks.
 @param configurations Optional configuration parameters for the underlying network SDKs that the providers manage. Only @c NSString, @c NSNumber, @c NSArray, and @c NSDictionary types are allowed. This value may be @c nil.
 @param options Optional MoPub request options for the mediated networks.
 @param complete Required completion block specifying the initialization error (if any) and the adapter information providers that were successfully initialized.
 */
- (void)initializeWithAdditionalProviders:(NSArray<Class<MPAdapterConfiguration>> * _Nullable)providers
                           configurations:(NSDictionary<NSString *, NSDictionary<NSString *, id> *> * _Nullable)configurations
                           requestOptions:(NSDictionary<NSString *, NSDictionary<NSString *, NSString *> *> * _Nullable)options
                                 complete:(MPMediationInitializationCompletionBlock)complete;

/**
 Sets the initialization parameters for a given network in the cache.
 @param params Initialization parameters sent to the @c MPSdkInitializable instance when initialized.
 @param networkClass Network class.
 */
- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params
                               forNetwork:(Class<MPAdapterConfiguration>)networkClass;

/**
 Retrieves the cached initialization parameters for a given network.
 @param networkClass Network class.
 @returns The cached parameters or @c nil.
 */
- (NSDictionary * _Nullable)cachedInitializationParametersForNetwork:(Class<MPAdapterConfiguration>)networkClass;

/**
 Clears the cache.
 */
- (void)clearCache;

/**
 Retrieves the Advanced Bidding tokens only.
 @remarks This is deprecated.
 */
- (NSDictionary<NSString *, NSDictionary *> * _Nullable)advancedBiddingTokens;

@end

NS_ASSUME_NONNULL_END

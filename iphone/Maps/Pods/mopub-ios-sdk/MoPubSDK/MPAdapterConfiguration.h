//
//  MPAdapterConfiguration.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol MPAdapterConfiguration <NSObject>
@required
/**
 The version of the adapter.
 */
@property (nonatomic, copy, readonly) NSString * adapterVersion;

/**
 An optional identity token used for ORTB bidding requests required for Advanced Bidding.
 */
@property (nonatomic, copy, readonly, nullable) NSString * biddingToken;

/**
 MoPub-specific name of the network.
 @remark This value should correspond to `creative_network_name` in the dashboard.
 */
@property (nonatomic, copy, readonly) NSString * moPubNetworkName;

/**
 Optional dictionary of additional values to send along with every MoPub ad request
 on behalf of the adapter.
 */
@property (nonatomic, readonly, nullable) NSDictionary<NSString *, NSString *> * moPubRequestOptions;

/**
 The version of the underlying network SDK.
 */
@property (nonatomic, copy, readonly) NSString * networkSdkVersion;

#pragma mark - Initialization

/**
 Initializes the underlying network SDK with a given set of initialization parameters.
 @param configuration Optional set of JSON-codable configuration parameters that correspond specifically to the network. Only @c NSString, @c NSNumber, @c NSArray, and @c NSDictionary types are allowed. This value may be @c nil.
 @param complete Optional completion block that is invoked when the underlying network SDK has completed initialization. This value may be @c nil.
 @remarks Classes that implement this protocol must account for the possibility of @c initializeNetworkWithConfiguration:complete: being called multiple times. It is up to each individual adapter to determine whether re-initialization is allowed or not.
 */
- (void)initializeNetworkWithConfiguration:(NSDictionary<NSString *, id> * _Nullable)configuration
                                  complete:(void(^ _Nullable)(NSError * _Nullable))complete;

#pragma mark - MoPub Request Options

/**
 Adds entries into the managed @c moPubRequestOptions dictionary, overwriting any previously set
 entries.
 @param options Entries to add into @c moPubRequestOptions. This should not be @c nil.
 */
- (void)addMoPubRequestOptions:(NSDictionary<NSString *, NSString *> *)options;

#pragma mark - Caching

/**
 Updates the initialization parameters for the current network.
 @param parameters New set of initialization parameters. Only @c NSString, @c NSNumber, @c NSArray, and @c NSDictionary types are allowed. Nothing will be done if @c nil is passed in.
 */
+ (void)setCachedInitializationParameters:(NSDictionary<NSString *, id> * _Nullable)parameters;

/**
 Retrieves the initialization parameters for the current network (if any).
 @return The cached initialization parameters for the network. This may be @c nil if not parameters were found.
 */
+ (NSDictionary<NSString *, id> * _Nullable)cachedInitializationParameters;

@end

NS_ASSUME_NONNULL_END

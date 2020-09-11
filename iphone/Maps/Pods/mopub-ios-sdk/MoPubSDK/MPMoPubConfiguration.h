//
//  MPMoPubConfiguration.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPAdapterConfiguration.h"
#import "MPBLogLevel.h"
#import "MPMediationSettingsProtocol.h"
#import "MPRewardedVideo.h"

NS_ASSUME_NONNULL_BEGIN

@interface MPMoPubConfiguration : NSObject
/**
 Optional list of additional mediated network SDKs to pre-initialize from the cache. If the mediated network
 SDK has no cache entry, nothing will be done.
 @remarks All certified mediated networks will be pre-initialized. This property is meant to pre-initialize
 any custom adapters that have not been certified by MoPub.
 */
@property (nonatomic, strong, nullable) NSArray<Class<MPAdapterConfiguration>> * additionalNetworks;

/**
 Any valid ad unit ID used within the app used for app initialization.
 @remark This is a required field.
 */
@property (nonatomic, strong, nonnull) NSString * adUnitIdForAppInitialization;

/**
 This API can be used if you want to allow supported SDK networks to collect user information on the basis of legitimate interest. The default value is @c NO.
 */
@property (nonatomic, assign) BOOL allowLegitimateInterest;

/**
 Optional global configurations for all ad networks your app supports.
 */
@property (nonatomic, strong, nullable) NSArray<id<MPMediationSettingsProtocol>> * globalMediationSettings;

/**
 Optional logging level. By default, this value is set to @c MPBLogLevelNone.
 */
@property (nonatomic, assign) MPBLogLevel loggingLevel;

/**
 Optional configuration settings for mediated networks during initialization. To add entries
 to this dictionary, use the convenience method @c setNetworkConfiguration:forMediationAdapter:
 */
@property (nonatomic, strong, nullable) NSMutableDictionary<NSString *, NSDictionary<NSString *, id> *> * mediatedNetworkConfigurations;

/**
 Optional MoPub request options for mediated networks. To add entries
 to this dictionary, use the convenience method @c setMoPubRequestOptions:forMediationAdapter:
 */
@property (nonatomic, strong, nullable) NSMutableDictionary<NSString *, NSDictionary<NSString *, NSString *> *> * moPubRequestOptions;

/**
 Initializes the @c MPMoPubConfiguration object with the required fields.
 @param adUnitId Any valid ad unit ID used within the app used for app initialization.
 @return A configuration instance.
 */
- (instancetype)initWithAdUnitIdForAppInitialization:(NSString *)adUnitId NS_DESIGNATED_INITIALIZER;

/**
 Sets the network configuration options for a given mediated network class name.
 @param configuration Configuration parameters specific to the network. Only @c NSString, @c NSNumber, @c NSArray, and @c NSDictionary types are allowed. This value may be @c nil.
 @param adapterClassName The class name of the mediated adapter that will receive the inputted configuration. The adapter class must implement the @c MPAdapterConfiguration protocol.
 */
- (void)setNetworkConfiguration:(NSDictionary<NSString *, id> * _Nullable)configuration
            forMediationAdapter:(NSString *)adapterClassName;

/**
 Sets the mediated network's MoPub request options.
 @param options MoPub request options for the mediated network.
 @param adapterClassName The class name of the mediated adapter that will receive the inputted configuration. The adapter class must implement the @c MPAdapterConfiguration protocol.
 */
- (void)setMoPubRequestOptions:(NSDictionary<NSString *, NSString *> * _Nullable)options
           forMediationAdapter:(NSString *)adapterClassName;

/**
 Usage of default initializer is disallowed. Use @c initWithAdUnitIdForAppInitialization: instead.
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Usage of @c new is disallowed. Use @c initWithAdUnitIdForAppInitialization: instead.
 */
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

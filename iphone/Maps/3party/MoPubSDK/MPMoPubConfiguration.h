//
//  MPMoPubConfiguration.h
//  MoPubSampleApp
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPAdvancedBidder.h"
#import "MPMediationSdkInitializable.h"
#import "MPMediationSettingsProtocol.h"
#import "MPRewardedVideo.h"

NS_ASSUME_NONNULL_BEGIN

@interface MPMoPubConfiguration : NSObject
/**
 Any valid ad unit ID used within the app used for app initialization.
 @remark This is a required field.
 */
@property (nonatomic, strong, nonnull) NSString * adUnitIdForAppInitialization;

/**
 Optional list of advanced bidders to initialize.
 */
@property (nonatomic, strong, nullable) NSArray<Class<MPAdvancedBidder>> * advancedBidders;

/**
 Optional global configurations for all ad networks your app supports.
 */
@property (nonatomic, strong, nullable) NSArray<id<MPMediationSettingsProtocol>> * globalMediationSettings;

/**
 Optional list of mediated network SDKs to pre-initialize from the cache. If the mediated network
 SDK has no cache entry, nothing will be done. If set to @c nil or empty array, no network
 SDKs will be preinitialized.

 To initialize all existing cached networks use @c MoPub.sharedInstance.allCachedNetworks
 */
@property (nonatomic, strong, nullable) NSArray<Class<MPMediationSdkInitializable>> * mediatedNetworks;

/**
 Initializes the @c MPMoPubConfiguration object with the required fields.
 @param adUnitId Any valid ad unit ID used within the app used for app initialization.
 @return A configuration instance.
 */
- (instancetype)initWithAdUnitIdForAppInitialization:(NSString *)adUnitId NS_DESIGNATED_INITIALIZER;

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

//
//  MPAdTargeting.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@class CLLocation;

NS_ASSUME_NONNULL_BEGIN

/**
 Optional targeting parameters to use when requesting an ad.
 */
@interface MPAdTargeting : NSObject

/**
 The maximum creative size that can be safely rendered in the ad container.
 The size should be in points.
 */
@property (nonatomic, assign) CGSize creativeSafeSize;

/**
 A string representing a set of non-personally identifiable keywords that should be passed
 to the MoPub ad server to receive more relevant advertising.

 @remark If a user is in General Data Protection Regulation (GDPR) region and MoPub doesn't obtain
 consent from the user, @c keywords will still be sent to the server.
 */
@property (nullable, nonatomic, copy) NSString * keywords;

/**
 Key-value pairs that are locally available to the custom event.
 */
@property (nullable, nonatomic, copy) NSDictionary * localExtras;

/**
 The device's current location if permission to access location has been granted.
 * @deprecated This API is deprecated and will be removed in a future version.
 */
@property (nullable, nonatomic, copy) CLLocation * location __attribute__((deprecated("This API is deprecated and will be removed in a future version.")));

/**
 A string representing a set of personally identifiable keywords that should be passed to the MoPub ad server to receive
 more relevant advertising.

 Keywords are typically used to target ad campaigns at specific user segments. They should be
 formatted as comma-separated key-value pairs (e.g. "marital:single,age:24").

 On the MoPub website, keyword targeting options can be found under the "Advanced Targeting"
 section when managing campaigns.

 @remark If a user is in General Data Protection Regulation (GDPR) region and MoPub doesn't obtain
 consent from the user, @c userDataKeywords will not be sent to the server.
 */
@property (nullable, nonatomic, copy) NSString * userDataKeywords;

/**
 Initializes ad targeting information.
 @param size The maximum creative size that can be safely rendered in the ad container.
 The size should be in points.
 */
- (instancetype)initWithCreativeSafeSize:(CGSize)size;

/**
 Initializes ad targeting information.
 @param size The maximum creative size that can be safely rendered in the ad container.
 The size should be in points.
 */
+ (instancetype)targetingWithCreativeSafeSize:(CGSize)size;

#pragma mark - Unavailable Initializers

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

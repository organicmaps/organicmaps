//
//  MPNativeAdRequestTargeting.h
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class CLLocation;

/**
 * The `MPNativeAdRequestTargeting` class is used to attach targeting information to
 * `MPNativeAdRequest` objects.
 */

@interface MPNativeAdRequestTargeting : NSObject

/** @name Creating a Targeting Object */

/**
 * Creates and returns an empty MPNativeAdRequestTargeting object.
 *
 * @return A newly initialized MPNativeAdRequestTargeting object.
 */
+ (MPNativeAdRequestTargeting *)targeting;

/** @name Targeting Parameters */

/**
 * A string representing a set of keywords that should be passed to the MoPub ad server to receive
 * more relevant advertising.
 *
 * Keywords are typically used to target ad campaigns at specific user segments. They should be
 * formatted as comma-separated key-value pairs (e.g. "marital:single,age:24").
 *
 * On the MoPub website, keyword targeting options can be found under the "Advanced Targeting"
 * section when managing campaigns.
 */
@property (nonatomic, copy) NSString *keywords;

/**
 * A `CLLocation` object representing a user's location that should be passed to the MoPub ad server
 * to receive more relevant advertising.
 */
@property (nonatomic, copy) CLLocation *location;

/**
 * A set of defined strings that correspond to assets for the intended native ad
 * object. This set should contain only those assets that will be displayed in the ad.
 *
 * The MoPub ad server will attempt to only return the assets in desiredAssets.
 */
@property (nonatomic, strong) NSSet *desiredAssets;

@end

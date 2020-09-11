//
//  MPNativeAdRequestTargeting.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPAdTargeting.h"

/**
 The @c MPNativeAdRequestTargeting class is used to attach targeting information to
 @c MPNativeAdRequest objects.
 */
@interface MPNativeAdRequestTargeting : MPAdTargeting

/**
 Creates and returns an empty @c MPNativeAdRequestTargeting object.
 @return A newly initialized @c MPNativeAdRequestTargeting object.
 */
+ (MPNativeAdRequestTargeting *)targeting;

/**
 A set of defined strings that correspond to assets for the intended native ad
 object. This set should contain only those assets that will be displayed in the ad.

 The MoPub ad server will attempt to only return the assets in @c desiredAssets.
 */
@property (nonatomic, strong) NSSet * desiredAssets;

@end

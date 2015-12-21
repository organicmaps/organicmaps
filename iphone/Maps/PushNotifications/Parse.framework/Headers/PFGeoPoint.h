/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class PFGeoPoint;

typedef void(^PFGeoPointResultBlock)(PFGeoPoint *__nullable geoPoint, NSError *__nullable error);

/**
 `PFGeoPoint` may be used to embed a latitude / longitude point as the value for a key in a `PFObject`.
 It could be used to perform queries in a geospatial manner using `PFQuery.-whereKey:nearGeoPoint:`.

 Currently, instances of `PFObject` may only have one key associated with a `PFGeoPoint` type.
 */
@interface PFGeoPoint : NSObject <NSCopying, NSCoding>

///--------------------------------------
/// @name Creating a Geo Point
///--------------------------------------

/**
 Create a PFGeoPoint object. Latitude and longitude are set to `0.0`.

 @return Returns a new `PFGeoPoint`.
 */
+ (instancetype)geoPoint;

/**
 Creates a new `PFGeoPoint` object for the given `CLLocation`, set to the location's coordinates.

 @param location Instace of `CLLocation`, with set latitude and longitude.

 @return Returns a new PFGeoPoint at specified location.
 */
+ (instancetype)geoPointWithLocation:(nullable CLLocation *)location;

/**
 Create a new `PFGeoPoint` object with the specified latitude and longitude.

 @param latitude Latitude of point in degrees.
 @param longitude Longitude of point in degrees.

 @return New point object with specified latitude and longitude.
 */
+ (instancetype)geoPointWithLatitude:(double)latitude longitude:(double)longitude;

/**
 Fetches the current device location and executes a block with a new `PFGeoPoint` object.

 @param resultBlock A block which takes the newly created `PFGeoPoint` as an argument.
 It should have the following argument signature: `^(PFGeoPoint *geoPoint, NSError *error)`
 */
+ (void)geoPointForCurrentLocationInBackground:(nullable PFGeoPointResultBlock)resultBlock;

///--------------------------------------
/// @name Controlling Position
///--------------------------------------

/**
 Latitude of point in degrees. Valid range is from `-90.0` to `90.0`.
 */
@property (nonatomic, assign) double latitude;

/**
 Longitude of point in degrees. Valid range is from `-180.0` to `180.0`.
 */
@property (nonatomic, assign) double longitude;

///--------------------------------------
/// @name Calculating Distance
///--------------------------------------

/**
 Get distance in radians from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @return Distance in radians between the receiver and `point`.
 */
- (double)distanceInRadiansTo:(nullable PFGeoPoint *)point;

/**
 Get distance in miles from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @return Distance in miles between the receiver and `point`.
 */
- (double)distanceInMilesTo:(nullable PFGeoPoint *)point;

/**
 Get distance in kilometers from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @return Distance in kilometers between the receiver and `point`.
 */
- (double)distanceInKilometersTo:(nullable PFGeoPoint *)point;

@end

NS_ASSUME_NONNULL_END

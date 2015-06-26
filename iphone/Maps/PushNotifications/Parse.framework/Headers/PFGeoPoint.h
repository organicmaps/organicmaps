//
//  PFGeoPoint.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#if TARGET_OS_IPHONE
#import <Parse/PFNullability.h>
#else
#import <ParseOSX/PFNullability.h>
#endif

PF_ASSUME_NONNULL_BEGIN

@class PFGeoPoint;

typedef void(^PFGeoPointResultBlock)(PFGeoPoint *PF_NULLABLE_S geoPoint, NSError *PF_NULLABLE_S error);

/*!
 `PFGeoPoint` may be used to embed a latitude / longitude point as the value for a key in a <PFObject>.
 It could be used to perform queries in a geospatial manner using <[PFQuery whereKey:nearGeoPoint:]>.

 Currently, instances of <PFObject> may only have one key associated with a `PFGeoPoint` type.
 */
@interface PFGeoPoint : NSObject <NSCopying, NSCoding>

///--------------------------------------
/// @name Creating a Geo Point
///--------------------------------------

/*!
 @abstract Create a PFGeoPoint object. Latitude and longitude are set to `0.0`.

 @returns Returns a new `PFGeoPoint`.
 */
+ (PFGeoPoint *)geoPoint;

/*!
 @abstract Creates a new `PFGeoPoint` object for the given `CLLocation`, set to the location's coordinates.

 @param location Instace of `CLLocation`, with set latitude and longitude.

 @returns Returns a new PFGeoPoint at specified location.
 */
+ (PFGeoPoint *)geoPointWithLocation:(PF_NULLABLE CLLocation *)location;

/*!
 @abstract Create a new `PFGeoPoint` object with the specified latitude and longitude.

 @param latitude Latitude of point in degrees.
 @param longitude Longitude of point in degrees.

 @returns New point object with specified latitude and longitude.
 */
+ (PFGeoPoint *)geoPointWithLatitude:(double)latitude longitude:(double)longitude;

/*!
 @abstract Fetches the current device location and executes a block with a new `PFGeoPoint` object.

 @param resultBlock A block which takes the newly created `PFGeoPoint` as an argument.
 It should have the following argument signature: `^(PFGeoPoint *geoPoint, NSError *error)`
 */
+ (void)geoPointForCurrentLocationInBackground:(PF_NULLABLE PFGeoPointResultBlock)resultBlock;

///--------------------------------------
/// @name Controlling Position
///--------------------------------------

/*!
 @abstract Latitude of point in degrees. Valid range is from `-90.0` to `90.0`.
 */
@property (nonatomic, assign) double latitude;

/*!
 @abstract Longitude of point in degrees. Valid range is from `-180.0` to `180.0`.
 */
@property (nonatomic, assign) double longitude;

///--------------------------------------
/// @name Calculating Distance
///--------------------------------------

/*!
 @abstract Get distance in radians from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @returns Distance in radians between the receiver and `point`.
 */
- (double)distanceInRadiansTo:(PF_NULLABLE PFGeoPoint *)point;

/*!
 @abstract Get distance in miles from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @returns Distance in miles between the receiver and `point`.
 */
- (double)distanceInMilesTo:(PF_NULLABLE PFGeoPoint *)point;

/*!
 @abstract Get distance in kilometers from this point to specified point.

 @param point `PFGeoPoint` that represents the location of other point.

 @returns Distance in kilometers between the receiver and `point`.
 */
- (double)distanceInKilometersTo:(PF_NULLABLE PFGeoPoint *)point;

@end

PF_ASSUME_NONNULL_END

//
//  PWRichMediaManager.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2018
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@class PWGeozonesManager;

@protocol PWGeozonesDelegate <NSObject>

@optional

/**
 Tells the delegate that location tracking did start.
 */
- (void)didStartLocationTrackingWithManager:(PWGeozonesManager *)geozonesManager;

/**
 Tells the delegate that location tracking did fail.
 */
- (void)geozonesManager:(PWGeozonesManager *)geozonesManager startingLocationTrackingDidFail:(NSError *)error;

/**
 Tells the delegate that location was successfully sent.
 */
- (void)geozonesManager:(PWGeozonesManager *)geozonesManager didSendLocation:(CLLocation *)location;

@end


@interface PWGeozonesManager: NSObject

/**
 Indicates that location tracking has started.
 */
@property (nonatomic, readonly) BOOL enabled;

/**
 Delegate that would receive the information about events for geozones manager.
 */
@property (nonatomic, weak) id<PWGeozonesDelegate> delegate;

/**
 A singleton object that represents the geozones manager.
 */
+ (instancetype)sharedManager;

/**
 Starts location tracking.
 */
- (void)startLocationTracking;

/**
 Stops location tracking.
 */
- (void)stopLocationTracking;

/**
 Explicitly sends geolocation to the server for GeoFencing push technology. Also called internally in `startLocationTracking` and `stopLocationTracking` functions.

 @param location Location to be sent.
 */
- (void)sendLocation:(CLLocation *)location;

@end

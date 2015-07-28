#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import <UIKit/UIApplication.h>

#include "../../platform/location.hpp"

#include "../../std/utility.hpp"


@protocol LocationObserver
@optional
- (void)onLocationError:(location::TLocationError)errorCode;
- (void)onLocationUpdate:(location::GpsInfo const &)info;
- (void)onCompassUpdate:(location::CompassInfo const &)info;
@end

@interface LocationManager : NSObject <CLLocationManagerDelegate>
{
  CLLocationManager * m_locationManager;
  BOOL m_isStarted;
  NSMutableSet * m_observers;
  NSDate * m_lastLocationTime;
  BOOL m_isCourse;
}

- (void)start:(id <LocationObserver>)observer;
- (void)stop:(id <LocationObserver>)observer;
- (CLLocation *)lastLocation;
- (CLHeading *)lastHeading;
// Fixes compass angle orientation when rotating screen to landscape
- (void)orientationChanged;

- (bool)getLat:(double &)lat Lon:(double &)lon;
- (bool)getNorthRad:(double &)rad;

+ (NSString *)formattedDistance:(double)meters;
// Returns nil if neither speed nor altitude are available
- (NSString *)formattedSpeedAndAltitude:(BOOL &)hasSpeed;

- (bool)lastLocationIsValid;
- (BOOL)enabledOnMap;
- (void)triggerCompass;

@end

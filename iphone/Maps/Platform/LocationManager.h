#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import <UIKit/UIApplication.h>

#include "../../platform/location.hpp"

#include "../../std/utility.hpp"


@protocol LocationObserver
@required
  - (void)onLocationStatusChanged:(location::TLocationStatus)newStatus;
  - (void)onGpsUpdate:(location::GpsInfo const &)info;
  - (void)onCompassUpdate:(location::CompassInfo const &)info;
@end

@interface LocationManager : NSObject <CLLocationManagerDelegate>
{
  CLLocationManager * m_locationManager;
  BOOL m_isStarted;
  BOOL m_reportFirstUpdate;
  NSMutableSet * m_observers;
  BOOL m_isTimerActive;

  // Fixed location coordinates for debug or other special purpose.
  pair<double, double> m_latlon;
  bool m_fixedLatLon;

  // Fixed device direction from north (radians).
  double m_dirFromNorth;
  bool m_fixedDir;
}

- (void)start:(id <LocationObserver>)observer;
- (void)stop:(id <LocationObserver>)observer;
- (CLLocation *)lastLocation;
- (CLHeading *)lastHeading;
// Fixes compass angle orientation when rotating screen to landscape
- (void)setOrientation:(UIInterfaceOrientation)orientation;

- (bool)getLat:(double &)lat Lon:(double &)lon;
- (bool)getNorthRad:(double &)rad;

+ (NSString *)formatDistance:(double)meters;
@end

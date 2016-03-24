#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import <UIKit/UIApplication.h>

#include "geometry/mercator.hpp"
#include "platform/location.hpp"
#include "std/utility.hpp"

@protocol LocationObserver
@required
- (void)onLocationUpdate:(location::GpsInfo const &)info;
@optional
- (void)onLocationError:(location::TLocationError)errorCode;
- (void)onCompassUpdate:(location::CompassInfo const &)info;
@end

@interface LocationManager : NSObject <CLLocationManagerDelegate>

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
- (bool)isLocationModeUnknownOrPending;
- (BOOL)enabledOnMap;
- (void)triggerCompass;

- (void)onDaemonMode;
- (void)onForeground;
- (void)onBackground;
- (void)beforeTerminate;

- (void)reset;

@end

@interface CLLocation (Mercator)

- (m2::PointD)mercator;

@end

static inline ms::LatLon ToLatLon(m2::PointD const & p)
{
  return MercatorBounds::ToLatLon(p);
}

static inline m2::PointD ToMercator(CLLocationCoordinate2D const & l)
{
  return MercatorBounds::FromLatLon(l.latitude, l.longitude);
}

static inline m2::PointD ToMercator(ms::LatLon const & l)
{
  return MercatorBounds::FromLatLon(l);
}

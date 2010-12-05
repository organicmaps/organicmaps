#import "UserLocationController.hpp"

#include "../../../indexer/mercator.hpp"

@implementation UserLocationController

@synthesize delegate;

- (id) init
{
  self = [super init];
  if (self != nil)
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
  }
  return self;
}

- (id) initWithDelegate: (id) locationDelegate
{
	delegate = locationDelegate;
	return [self init];
}

- (void) dealloc
{
	[self Stop];
  [m_locationManager release];
  [super dealloc];
}

- (void) Start
{
  [m_locationManager startUpdatingLocation];
}

- (void) Stop
{
	[m_locationManager stopUpdatingLocation];
}

- (void) locationManager: (CLLocationManager *) manager
     didUpdateToLocation: (CLLocation *) newLocation
            fromLocation: (CLLocation *) oldLocation
{
	m2::PointD mercPoint(MercatorBounds::LonToX(newLocation.coordinate.longitude),
  										 MercatorBounds::LatToY(newLocation.coordinate.latitude));
	[self.delegate OnLocation: mercPoint withTimestamp: newLocation.timestamp];
}

- (void) locationManager: (CLLocationManager *) manager
	      didFailWithError: (NSError *) error
{
	[self.delegate OnLocationError: [error localizedDescription]];
}

@end
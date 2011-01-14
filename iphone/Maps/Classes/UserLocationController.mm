#import "UserLocationController.hpp"

#include "../../../indexer/mercator.hpp"

@implementation UserLocationController

@synthesize delegate;
@synthesize active;

- (id) init
{
  self = [super init];
  if (self != nil)
  {
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    active = NO;
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
	active = YES;
  [m_locationManager startUpdatingLocation];
	if ([m_locationManager headingAvailable])
	{
		m_locationManager.headingFilter = 1;
		[m_locationManager startUpdatingHeading];
	}
	else
		NSLog(@"heading information is not available");
}

- (void) Stop
{
	[m_locationManager stopUpdatingLocation];
	[m_locationManager stopUpdatingHeading];
  active = NO;
}

- (void) locationManager: (CLLocationManager *) manager
				didUpdateHeading:	(CLHeading *) newHeading
{
	[self.delegate OnHeading: newHeading];
}

- (void) locationManager: (CLLocationManager *) manager
     didUpdateToLocation: (CLLocation *) newLocation
            fromLocation: (CLLocation *) oldLocation
{
	m2::PointD mercPoint(MercatorBounds::LonToX(newLocation.coordinate.longitude),
  										 MercatorBounds::LatToY(newLocation.coordinate.latitude));
	
	double confidenceRadius = sqrt(newLocation.horizontalAccuracy * newLocation.horizontalAccuracy 
															 + newLocation.verticalAccuracy * newLocation.verticalAccuracy);
	
	m2::RectD errorRect = MercatorBounds::ErrorToRadius(newLocation.coordinate.longitude, 
																											newLocation.coordinate.latitude,
																											confidenceRadius);
	
	confidenceRadius = sqrt((errorRect.SizeX() * errorRect.SizeX() + errorRect.SizeY() * errorRect.SizeY()) / 4);
	
	[self.delegate OnLocation: mercPoint withConfidenceRadius: confidenceRadius withTimestamp: newLocation.timestamp];
}

- (void) locationManager: (CLLocationManager *) manager
	      didFailWithError: (NSError *) error
{
	[self.delegate OnLocationError: [error localizedDescription]];
}

@end
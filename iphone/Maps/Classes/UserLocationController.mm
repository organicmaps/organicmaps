#import "UserLocationController.h"

#include "../../../indexer/mercator.hpp"

@implementation UserLocationController

@synthesize delegate;
//@synthesize active;
@synthesize locationManager;

- (id) init
{
  self = [super init];
  if (self != nil)
  {
    self.locationManager = [[CLLocationManager alloc] init];
    self.locationManager.delegate = self;
//    active = NO;
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
  [self.locationManager release];
  [super dealloc];
}

- (void) Start
{
//	active = YES;
  [self.locationManager startUpdatingLocation];
	if ([CLLocationManager headingAvailable])
	{
		self.locationManager.headingFilter = 1;
    [self.locationManager startUpdatingHeading];
	}
	else
		NSLog(@"heading information is not available");
}

- (void) Stop
{
	[self.locationManager stopUpdatingLocation];
	[self.locationManager stopUpdatingHeading];
//  active = NO;
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
//  NSLog(@"NewLocation: %@", [newLocation description]);
	m2::PointD mercPoint(MercatorBounds::LonToX(newLocation.coordinate.longitude),
                       MercatorBounds::LatToY(newLocation.coordinate.latitude));

  m2::RectD errorRectXY = MercatorBounds::MetresToXY(newLocation.coordinate.longitude,
                                                     newLocation.coordinate.latitude,
                                                     newLocation.horizontalAccuracy);

  double errorRadiusXY = sqrt((errorRectXY.SizeX() * errorRectXY.SizeX()
                           + errorRectXY.SizeY() * errorRectXY.SizeY()) / 4);

  [self.delegate OnLocation: mercPoint withErrorRadius: errorRadiusXY withTimestamp: newLocation.timestamp];
}

- (void) locationManager: (CLLocationManager *) manager
	      didFailWithError: (NSError *) error
{
	[self.delegate OnLocationError: [error localizedDescription]];
}

@end

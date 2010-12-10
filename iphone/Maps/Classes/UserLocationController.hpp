#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include "../../../geometry/point2d.hpp"

@protocol UserLocationControllerDelegate 

@required
- (void) OnLocation: (m2::PointD const &) mercatorPoint 
withConfidenceRadius: (double) confidenceRadius
			withTimestamp: (NSDate *) timestamp;
- (void) OnHeading: (double) heading
		 withTimestamp: (NSDate *) timestamp;
- (void) OnLocationError: (NSString *) errorDescription;
@end

@interface UserLocationController : NSObject<CLLocationManagerDelegate>
{
@private
	CLLocationManager * m_locationManager;
  
@public
	id delegate;
}

@property (nonatomic, assign) id delegate;

- (id) initWithDelegate: (id) locationDelegate;

- (void) Start;

- (void) Stop;

- (void) locationManager: (CLLocationManager *) manager
     didUpdateToLocation: (CLLocation *) newLocation
            fromLocation: (CLLocation *) oldLocation;

- (void) locationManager: (CLLocationManager *) manager
				didUpdateHeading: (CLHeading *) newHeading;

- (void) locationManager: (CLLocationManager *) manager
        didFailWithError: (NSError *) error;

@end

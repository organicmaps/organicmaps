#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include "../../../geometry/point2d.hpp"

@protocol UserLocationControllerDelegate

@required
- (void) OnLocation: (m2::PointD const &) mercatorPoint
withConfidenceRadius: (double) confidenceRadius
			withTimestamp: (NSDate *) timestamp;
- (void) OnHeading: (CLHeading *)heading;
- (void) OnLocationError: (NSString *) errorDescription;
@end

@interface UserLocationController : NSObject<CLLocationManagerDelegate>
{
@private
	CLLocationManager * m_locationManager;

@public
	id delegate;
  BOOL active;
}

@property (nonatomic, assign) id delegate;
/// YES means that location manager is active
@property (nonatomic, assign) BOOL active;

- (id) initWithDelegate: (id) locationDelegate;
- (void) Start;
- (void) Stop;

@end

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(GeoUtil)
@interface MWMGeoUtil : NSObject

+ (float)angleAtPoint:(CLLocationCoordinate2D)p1 toPoint:(CLLocationCoordinate2D)p2;

@end

NS_ASSUME_NONNULL_END

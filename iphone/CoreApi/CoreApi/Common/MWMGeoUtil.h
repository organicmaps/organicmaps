#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Measure : NSObject

@property(nonatomic, readonly) double value;
@property(nonatomic, readonly) NSString* valueAsString;

@property(nonatomic, readonly) NSString* unit;

- (instancetype) initAsSpeed:(double) mps;

@end

NS_SWIFT_NAME(GeoUtil)
@interface MWMGeoUtil : NSObject

+ (float)angleAtPoint:(CLLocationCoordinate2D)p1 toPoint:(CLLocationCoordinate2D)p2;

@end

NS_ASSUME_NONNULL_END

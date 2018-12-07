#import "IMWMGeoTrackerZone.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IMWMGeoTrackerCore

- (NSArray<id<IMWMGeoTrackerZone>> *)geoZonesForLat:(CLLocationDegrees)lat
                                                lon:(CLLocationDegrees)lon
                                           accuracy:(CLLocationAccuracy) accuracy;

- (void)logEnterZone:(id<IMWMGeoTrackerZone>)zone location:(CLLocation *)location;

@end

NS_ASSUME_NONNULL_END

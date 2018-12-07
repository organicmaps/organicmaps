NS_ASSUME_NONNULL_BEGIN

@protocol IMWMGeoTrackerZone<NSObject>

@property (nonatomic, readonly) CLLocationDegrees latitude;
@property (nonatomic, readonly) CLLocationDegrees longitude;
@property (nonatomic, readonly) NSString *identifier;

@end

NS_ASSUME_NONNULL_END

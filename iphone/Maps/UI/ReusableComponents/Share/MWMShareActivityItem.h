@class PlacePageData;

@interface MWMShareActivityItem : NSObject

@property(nonatomic, readonly) id<UIActivityItemsConfigurationReading> activityItemsConfiguration;

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location;
- (instancetype)initForPlacePage:(PlacePageData *)data;

@end

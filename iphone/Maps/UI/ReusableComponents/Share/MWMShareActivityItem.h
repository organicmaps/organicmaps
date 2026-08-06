@class PlacePageData;

@interface MWMShareActivityItem : NSObject

@property(nonatomic, readonly) NSArray * activityItems;

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location;
- (instancetype)initForPlacePage:(PlacePageData *)data;

@end

@class PlacePageData;

@interface MWMShareActivityItem : NSObject<UIActivityItemSource>

- (instancetype _Nullable)initForMyPositionAtLocation:(CLLocationCoordinate2D)location;
- (instancetype _Nullable)initForPlacePage:(PlacePageData * _Nonnull)data;

@end

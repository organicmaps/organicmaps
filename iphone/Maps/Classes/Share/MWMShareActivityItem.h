@class MWMPlacePageEntity;

@interface MWMShareActivityItem : NSObject<UIActivityItemSource>

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location;
- (instancetype)initForPlacePageObjectWithEntity:(MWMPlacePageEntity *)entity;

@end

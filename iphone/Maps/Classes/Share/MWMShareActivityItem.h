namespace ms
{
class LatLon;
}  // namespace ms

@protocol MWMPlacePageObject<NSObject>

- (BOOL)isMyPosition;
- (NSString *)title;
- (NSString *)subtitle;
- (NSString *)address;
- (NSString *)phoneNumber;
- (ms::LatLon)latLon;

@end

@class PlacePageData;

@interface MWMShareActivityItem : NSObject<UIActivityItemSource>

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location;
- (instancetype)initForPlacePageObject:(id<MWMPlacePageObject>)object;
- (instancetype)initForPlacePage:(PlacePageData *)data;

@end

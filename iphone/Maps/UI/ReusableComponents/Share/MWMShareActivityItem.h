@interface MWMShareActivityItem : NSObject

@property(nonatomic, readonly) NSArray<id<UIActivityItemSource>> * activityItems;

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location;
// The place page is open, so the core has the info (with metadata) to build the shared text.
- (instancetype)initForCurrentPlacePage;

@end

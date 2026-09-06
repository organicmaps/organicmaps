NS_ASSUME_NONNULL_BEGIN

@interface MWMCarPlaySearchResultObject : NSObject
@property(strong, nonatomic, readonly) NSString * title;
@property(strong, nonatomic, readonly) NSString * address;
@property(assign, nonatomic, readonly) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readonly) CGPoint mercatorPoint;

- (instancetype)initForRow:(NSInteger)row;
@end

NS_ASSUME_NONNULL_END

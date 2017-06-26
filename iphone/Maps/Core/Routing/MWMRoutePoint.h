typedef NS_ENUM(NSUInteger, MWMRoutePointType) {
  MWMRoutePointTypeStart,
  MWMRoutePointTypeIntermediate,
  MWMRoutePointTypeFinish
};

@interface MWMRoutePoint : NSObject

- (instancetype)initWithLastLocationAndType:(MWMRoutePointType)type;

@property(copy, nonatomic, readonly) NSString * name;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic, readonly) MWMRoutePointType type;

@property(nonatomic, readonly) double latitude;
@property(nonatomic, readonly) double longitude;

@end

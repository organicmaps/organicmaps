typedef NS_ENUM(NSUInteger, MWMRoutePointType) {
  MWMRoutePointTypeStart,
  MWMRoutePointTypeIntermediate,
  MWMRoutePointTypeFinish
};

@interface MWMRoutePoint : NSObject

- (instancetype)initWithLastLocationAndType:(MWMRoutePointType)type;

@property(copy, nonatomic, readonly) NSString * title;
@property(copy, nonatomic, readonly) NSString * subtitle;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic, readonly) MWMRoutePointType type;
@property(nonatomic, readonly) int8_t intermediateIndex;

@property(nonatomic, readonly) double latitude;
@property(nonatomic, readonly) double longitude;

@end

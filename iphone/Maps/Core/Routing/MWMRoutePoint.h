typedef NS_ENUM(NSUInteger, MWMRoutePointType) {
  MWMRoutePointTypeStart,
  MWMRoutePointTypeIntermediate,
  MWMRoutePointTypeFinish
};

@interface MWMRoutePoint : NSObject

- (instancetype)initWithLastLocationAndType:(MWMRoutePointType)type
                          intermediateIndex:(size_t)intermediateIndex;

- (instancetype)initWithCGPoint:(CGPoint)point
                          title:(NSString *)title
                       subtitle:(NSString *)subtitle
                           type:(MWMRoutePointType)type
              intermediateIndex:(size_t)intermediateIndex;

@property(copy, nonatomic, readonly) NSString * title;
@property(copy, nonatomic, readonly) NSString * subtitle;
@property(copy, nonatomic, readonly) NSString * latLonString;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic) MWMRoutePointType type;
@property(nonatomic) size_t intermediateIndex;

@property(nonatomic, readonly) double latitude;
@property(nonatomic, readonly) double longitude;

@end

typedef NS_CLOSED_ENUM(NSUInteger, MWMRoutePointType) {
  MWMRoutePointTypeStart,
  MWMRoutePointTypeIntermediate,
  MWMRoutePointTypeFinish
};

NS_ASSUME_NONNULL_BEGIN

@interface MWMRoutePoint : NSObject

- (nullable instancetype)initWithLastLocationAndType:(MWMRoutePointType)type
                                   intermediateIndex:(size_t)intermediateIndex;

- (instancetype)initWithCGPoint:(CGPoint)point
                          title:(nullable NSString *)title
                       subtitle:(nullable NSString *)subtitle
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

@interface MWMRoutePointSelection : NSObject

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithPoint:(nullable MWMRoutePoint *)point
                         type:(MWMRoutePointType)type
                 shouldAppend:(BOOL)shouldAppend;

@property(nonatomic, strong, readonly, nullable) MWMRoutePoint * point;
@property(nonatomic, readonly) MWMRoutePointType type;
@property(nonatomic, readonly) BOOL shouldAppend;

@end

NS_ASSUME_NONNULL_END

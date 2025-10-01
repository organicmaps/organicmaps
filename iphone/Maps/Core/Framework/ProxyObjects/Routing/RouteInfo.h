@class MWMRouterTransitStepInfo;

@interface RouteInfo : NSObject

@property(nonatomic, readonly) double distanceToTurn;
@property(nonatomic, readonly) NSString * distanceToTurnString;
@property(nonatomic, readonly) double targetDistance;
@property(nonatomic, readonly) NSString * targetDistanceString;
@property(nonatomic, readonly) NSUnitLength * targetUnits;
@property(nonatomic, readonly) NSString * targetUnitsString;
@property(nonatomic, readonly) NSUnitLength * turnUnits;
@property(nonatomic, readonly) NSString * turnUnitsString;
@property(nonatomic, readonly) NSString * streetName;
@property(nonatomic, readonly) double currentSpeedMps;
@property(nonatomic, readonly) double speedLimitMps;
@property(nonatomic, readonly) CGFloat progress;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
@property(nonatomic, readonly) NSUInteger timeToTarget;
@property(copy, nonatomic, readonly) NSString * nextTurnImageName;
@property(copy, nonatomic, readonly) NSString * turnImageName;
@property(copy, nonatomic, readonly) NSArray<MWMRouterTransitStepInfo *> * transitSteps;

@property(nonatomic, readonly) NSString * arrival;

- (NSAttributedString *)estimate;

+ (NSAttributedString *)estimateDot;

+ (instancetype)new __attribute__((unavailable("init is not available")));

@end

@class MWMRouterTransitStepInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic, readonly) NSString * distanceToTurn;
@property(copy, nonatomic, readonly) NSString * streetName;
@property(copy, nonatomic, readonly) NSString * targetDistance;
@property(copy, nonatomic, readonly) NSString * targetUnits;
@property(copy, nonatomic, readonly) NSString * turnUnits;
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

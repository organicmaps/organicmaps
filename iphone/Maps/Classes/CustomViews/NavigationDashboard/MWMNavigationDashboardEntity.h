@class MWMRouterTransitStepInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic, readonly) NSArray<MWMRouterTransitStepInfo *> *transitSteps;
@property(copy, nonatomic, readonly) NSAttributedString *estimate;
@property(copy, nonatomic, readonly) NSString *distanceToTurn;
@property(copy, nonatomic, readonly) NSString *streetName;
@property(copy, nonatomic, readonly) NSString *targetDistance;
@property(copy, nonatomic, readonly) NSString *targetUnits;
@property(copy, nonatomic, readonly) NSString *turnUnits;
@property(nonatomic, readonly) double speedLimitMps;
@property(nonatomic, readonly) BOOL isValid;
@property(nonatomic, readonly) CGFloat progress;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
@property(nonatomic, readonly) NSUInteger timeToTarget;
@property(nonatomic, readonly) UIImage *nextTurnImage;
@property(nonatomic, readonly) UIImage *turnImage;

@property(nonatomic, readonly) NSString * arrival;

+ (NSAttributedString *) estimateDot;

+ (instancetype) new __attribute__((unavailable("init is not available")));

@end

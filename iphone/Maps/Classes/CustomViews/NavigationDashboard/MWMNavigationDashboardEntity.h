@class MWMRouterTransitStepInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic) NSArray<MWMRouterTransitStepInfo *> * transitSteps;
@property(copy, nonatomic) NSAttributedString * estimate;
@property(copy, nonatomic, readonly) NSAttributedString * estimateDot;
@property(copy, nonatomic) NSString * distanceToTurn;
@property(nonatomic) NSUInteger timeToTarget;
@property(copy, nonatomic) NSString * streetName;
@property(copy, nonatomic) NSString * targetDistance;
@property(copy, nonatomic) NSString * targetUnits;
@property(copy, nonatomic) NSString * turnUnits;
@property(nonatomic) double speedLimitMps;
@property(nonatomic) BOOL isValid;
@property(nonatomic) CGFloat progress;
@property(nonatomic, readonly) NSString * arrival;
@property(nonatomic, readonly) NSString * eta;
@property(nonatomic) NSUInteger roundExitNumber;
@property(nonatomic) UIImage * nextTurnImage;
@property(nonatomic) UIImage * turnImage;
@property(nonatomic, readonly) BOOL isSpeedCamLimitExceeded;

+ (instancetype) new __attribute__((unavailable("init is not available")));

@end

@class MWMRouterTransitStepInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic, readonly) NSArray<MWMRouterTransitStepInfo *> * transitSteps;
@property(copy, nonatomic, readonly) NSString * distanceToTurn;
@property(copy, nonatomic, readonly) NSString * streetName;
@property(copy, nonatomic, readonly) NSString * targetDistance;
@property(copy, nonatomic, readonly) NSString * targetUnits;
@property(copy, nonatomic, readonly) NSString * turnUnits;
@property(nonatomic, readonly) double speedLimitMps;
@property(nonatomic, readonly) CGFloat progress;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
/// Remaining time in seconds; duration labels round it without affecting the arrival calculation.
@property(nonatomic, readonly) NSUInteger timeToTarget;
@property(nonatomic, readonly) UIImage * nextTurnImage;
@property(nonatomic, readonly) UIImage * turnImage;

@property(nonatomic, readonly) NSString * arrival;

- (NSAttributedString *)estimate;

- (instancetype)initWithWaterDistance:(double)waterDistance
                         landDistance:(double)landDistance
                    shortestConfirmed:(BOOL)shortestConfirmed;

+ (NSAttributedString *)estimateDot;

+ (instancetype)new __attribute__((unavailable("init is not available")));

@end

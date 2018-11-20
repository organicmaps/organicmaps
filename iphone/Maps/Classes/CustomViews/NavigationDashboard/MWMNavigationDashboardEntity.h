@class MWMRouterTransitStepInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic, readonly) NSArray<MWMRouterTransitStepInfo *> * transitSteps;
@property(copy, nonatomic, readonly) NSAttributedString * estimate;
@property(copy, nonatomic, readonly) NSAttributedString * estimateDot;
@property(copy, nonatomic, readonly) NSString * distanceToTurn;
@property(copy, nonatomic, readonly) NSString * streetName;
@property(copy, nonatomic, readonly) NSString * targetDistance;
@property(copy, nonatomic, readonly) NSString * targetUnits;
@property(copy, nonatomic, readonly) NSString * turnUnits;
@property(nonatomic, readonly) BOOL isValid;
@property(nonatomic, readonly) CGFloat progress;
@property(nonatomic, readonly) CLLocation * pedestrianDirectionPosition;
@property(nonatomic, readonly) NSString * arrival;
@property(nonatomic, readonly) NSString * eta;
@property(nonatomic, readonly) NSString * speed;
@property(nonatomic, readonly) NSString * speedUnits;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
@property(nonatomic, readonly) UIImage * nextTurnImage;
@property(nonatomic, readonly) UIImage * turnImage;
@property(nonatomic, readonly) BOOL isSpeedLimitExceeded;

+ (instancetype) new __attribute__((unavailable("init is not available")));

@end

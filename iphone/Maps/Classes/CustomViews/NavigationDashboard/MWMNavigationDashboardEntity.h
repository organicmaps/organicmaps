@interface MWMNavigationDashboardEntity : NSObject

@property(nonatomic, readonly) CLLocation * pedestrianDirectionPosition;
@property(nonatomic, readonly) BOOL isValid;
@property(nonatomic, readonly) NSString * speed;
@property(nonatomic, readonly) NSString * speedUnits;
@property(nonatomic, readonly) NSString * targetDistance;
@property(nonatomic, readonly) NSString * targetUnits;
@property(nonatomic, readonly) NSString * distanceToTurn;
@property(nonatomic, readonly) NSString * turnUnits;
@property(nonatomic, readonly) NSString * streetName;
@property(nonatomic, readonly) UIImage * turnImage;
@property(nonatomic, readonly) UIImage * nextTurnImage;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
@property(nonatomic, readonly) NSString * eta;
@property(nonatomic, readonly) NSString * arrival;
@property(nonatomic, readonly) CGFloat progress;
//@property (nonatomic, readonly) vector<location::FollowingInfo::SingleLaneInfoClient> lanes;
@property(nonatomic, readonly) BOOL isPedestrian;
@property(nonatomic, readonly) NSAttributedString * estimate;

+ (instancetype) new __attribute__((unavailable("init is not available")));

@end

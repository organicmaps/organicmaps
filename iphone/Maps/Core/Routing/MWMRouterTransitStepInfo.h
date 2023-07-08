#import "MWMRouterTransitType.h"

@interface MWMRouterTransitStepInfo : NSObject

@property(nonatomic, readonly) MWMRouterTransitType type;
@property(copy, nonatomic, readonly) NSString * distance;
@property(copy, nonatomic, readonly) NSString * distanceUnits;
@property(copy, nonatomic, readonly) NSString * number;
@property(nonatomic, readonly) UIColor * color;
@property(nonatomic, readonly) NSInteger intermediateIndex;

@end

#import "MWMRouterTransitType.h"

@interface MWMRouterTransitStepInfo : NSObject

@property(nonatomic, readwrite) MWMRouterTransitType type;
@property(copy, nonatomic, readwrite) NSString * distance;
@property(copy, nonatomic, readwrite) NSString * distanceUnits;
@property(copy, nonatomic, readwrite) NSString * number;
@property(nonatomic, readwrite) UIColor * color;
@property(nonatomic, readwrite) NSInteger intermediateIndex;

@end

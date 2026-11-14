#import "MWMRouterTransitType.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMRouterTransitStepInfo : NSObject

@property(nonatomic, readwrite) MWMRouterTransitType type;
@property(copy, nonatomic, readwrite) NSString * distance;
@property(copy, nonatomic, readwrite) NSString * distanceUnits;
@property(copy, nonatomic, readwrite) NSString * number;
@property(nonatomic, readwrite) UIColor * color;
@property(nonatomic, readwrite) NSInteger intermediateIndex;
@property(nonatomic, readwrite) NSInteger timeInSec;

// Valid for transit line steps only. The routed line: the stops below belong to it, while number may also
// list parallel lines.
@property(copy, nonatomic, readwrite) NSString * lineNumber;
// Empty when the stop has no name.
@property(copy, nonatomic, readwrite) NSString * startStopName;
@property(copy, nonatomic, readwrite) NSString * endStopName;
@property(copy, nonatomic, readwrite) NSArray<NSString *> * intermediateStopNames;
@property(nonatomic, readwrite) NSInteger stopCount;

@end

NS_ASSUME_NONNULL_END

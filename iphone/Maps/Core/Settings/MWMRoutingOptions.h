#import <Foundation/Foundation.h>

#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, MWMRoutingAvoidance) {
  MWMRoutingAvoidanceToll = 1u << 1,
  MWMRoutingAvoidanceMotorway = 1u << 2,
  MWMRoutingAvoidanceFerry = 1u << 3,
  MWMRoutingAvoidanceDirty = 1u << 4
} NS_SWIFT_NAME(RoutingAvoidance);

NS_SWIFT_NAME(RoutingOptions)
@interface MWMRoutingOptions : NSObject

@property(nonatomic, readonly) MWMRouterType routerType;
@property(nonatomic, readonly) MWMRoutingAvoidance supportedOptions;

- (instancetype)initWithRouterType:(MWMRouterType)routerType NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@property(nonatomic) BOOL avoidToll;
@property(nonatomic) BOOL avoidDirty;
@property(nonatomic) BOOL avoidFerry;
@property(nonatomic) BOOL avoidMotorway;
/// Persisted immediately, unlike the avoidance options, which need -save.
@property(nonatomic) BOOL routeOptimizationEnabled;
@property(nonatomic, readonly) BOOL hasOptions;

- (void)save;

@end

NS_ASSUME_NONNULL_END

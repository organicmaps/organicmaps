#import <Foundation/Foundation.h>

#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, MWMRoutingAvoidance) {
  MWMRoutingAvoidanceToll = 1u << 1,
  MWMRoutingAvoidanceMotorway = 1u << 2,
  MWMRoutingAvoidanceFerry = 1u << 3,
  MWMRoutingAvoidanceDirty = 1u << 4,
  MWMRoutingAvoidanceSteps = 1u << 5,
  MWMRoutingAvoidanceDifficultTrail = 1u << 6
} NS_SWIFT_NAME(RoutingAvoidance);

typedef NS_ENUM(NSInteger, MWMBicycleSurfacePreference) {
  MWMBicycleSurfacePreferenceAny = 0,
  MWMBicycleSurfacePreferencePreferPaved = 1,
  MWMBicycleSurfacePreferencePreferUnpaved = 2,
  MWMBicycleSurfacePreferenceAvoidUnpaved = 3
} NS_SWIFT_NAME(BicycleSurfacePreference);

typedef NS_OPTIONS(NSUInteger, MWMTransitModes) {
  MWMTransitModesBus = 1u << 0,
  MWMTransitModesTram = 1u << 1,
  MWMTransitModesSubway = 1u << 2,
  MWMTransitModesRail = 1u << 3,
  MWMTransitModesFerry = 1u << 4,
  MWMTransitModesTrolleybus = 1u << 5,
  MWMTransitModesMonorail = 1u << 6,
  MWMTransitModesCable = 1u << 7,
  MWMTransitModesAir = 1u << 8,
  MWMTransitModesAll = MWMTransitModesBus | MWMTransitModesTram | MWMTransitModesSubway | MWMTransitModesRail |
      MWMTransitModesFerry | MWMTransitModesTrolleybus | MWMTransitModesMonorail | MWMTransitModesCable |
      MWMTransitModesAir
} NS_SWIFT_NAME(TransitModes);

NS_SWIFT_NAME(RoutingOptions)
@interface MWMRoutingOptions : NSObject

@property(nonatomic, readonly) MWMRouterType routerType;
@property(nonatomic, readonly) MWMRoutingAvoidance supportedOptions;
@property(class, nonatomic) MWMBicycleSurfacePreference bicycleSurfacePreference;
@property(class, nonatomic) MWMTransitModes transitModes;

- (instancetype)initWithRouterType:(MWMRouterType)routerType NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@property(nonatomic) BOOL avoidToll;
@property(nonatomic) BOOL avoidDirty;
@property(nonatomic) BOOL avoidFerry;
@property(nonatomic) BOOL avoidMotorway;
@property(nonatomic) BOOL avoidSteps;
@property(nonatomic) BOOL avoidDifficultTrails;
/// Persisted immediately, unlike the avoidance options, which need -save.
@property(nonatomic) BOOL routeOptimizationEnabled;
@property(nonatomic, readonly) BOOL hasOptions;
@property(nonatomic, readonly) NSInteger enabledOptionsCount;

- (void)save;

@end

NS_ASSUME_NONNULL_END

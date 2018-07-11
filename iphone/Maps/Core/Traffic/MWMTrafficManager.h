typedef NS_ENUM(NSUInteger, MWMTrafficManagerState) {
  MWMTrafficManagerStateDisabled,
  MWMTrafficManagerStateEnabled,
  MWMTrafficManagerStateWaitingData,
  MWMTrafficManagerStateOutdated,
  MWMTrafficManagerStateNoData,
  MWMTrafficManagerStateNetworkError,
  MWMTrafficManagerStateExpiredData,
  MWMTrafficManagerStateExpiredApp
};

typedef NS_ENUM(NSUInteger, MWMTransitManagerState) {
  MWMTransitManagerStateDisabled,
  MWMTransitManagerStateEnabled,
  MWMTransitManagerStateNoData,
};

@protocol MWMTrafficManagerObserver<NSObject>

- (void)onTrafficStateUpdated;

@optional
- (void)onTransitStateUpdated;

@end

@interface MWMTrafficManager : NSObject

+ (void)addObserver:(id<MWMTrafficManagerObserver>)observer;
+ (void)removeObserver:(id<MWMTrafficManagerObserver>)observer;

+ (MWMTrafficManagerState)trafficState;
+ (MWMTransitManagerState)transitState;

+ (BOOL)trafficEnabled;
+ (BOOL)transitEnabled;

+ (void)setTrafficEnabled:(BOOL)enable;
+ (void)setTransitEnabled:(BOOL)enable;

@end

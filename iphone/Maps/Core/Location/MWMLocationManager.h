#import "MWMMyPositionMode.h"
#import "MWMLocationObserver.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LocationService

+ (BOOL)isLocationProhibited;
+ (void)checkLocationStatus;

@end

NS_SWIFT_NAME(LocationManager)
@interface MWMLocationManager : NSObject<LocationService>

+ (void)start;
+ (void)stop;
+ (BOOL)isStarted;

+ (void)addObserver:(id<MWMLocationObserver>)observer NS_SWIFT_NAME(add(observer:));
+ (void)removeObserver:(id<MWMLocationObserver>)observer NS_SWIFT_NAME(remove(observer:));

+ (void)setMyPositionMode:(MWMMyPositionMode)mode;

+ (nullable CLLocation *)lastLocation;
+ (nullable CLHeading *)lastHeading;

+ (void)applicationDidBecomeActive;
+ (void)applicationWillResignActive;

+ (void)enableLocationAlert;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +manager instead")));
+ (instancetype) new __attribute__((unavailable("call +manager instead")));

@end

NS_ASSUME_NONNULL_END

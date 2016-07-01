#import "CLLocation+Mercator.h"

#include "platform/location.hpp"

@protocol MWMLocationObserver <NSObject>

@optional
- (void)onHeadingUpdate:(location::CompassInfo const &)compassinfo;
- (void)onLocationUpdate:(location::GpsInfo const &)gpsInfo;
- (void)onLocationError:(location::TLocationError)locationError;

@end

@interface MWMLocationManager : NSObject

+ (void)addObserver:(id<MWMLocationObserver>)observer;
+ (void)removeObserver:(id<MWMLocationObserver>)observer;

+ (void)setMyPositionMode:(location::EMyPositionMode)mode;

+ (CLLocation *)lastLocation;
+ (location::TLocationError)lastLocationStatus;
+ (CLHeading *)lastHeading;

+ (void)applicationDidBecomeActive;
+ (void)applicationWillResignActive;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)alloc __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)new __attribute__((unavailable("call +manager instead")));

@end

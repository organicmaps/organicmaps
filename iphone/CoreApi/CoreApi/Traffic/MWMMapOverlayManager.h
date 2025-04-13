#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, MWMMapOverlayTrafficState) {
  MWMMapOverlayTrafficStateDisabled,
  MWMMapOverlayTrafficStateEnabled,
  MWMMapOverlayTrafficStateWaitingData,
  MWMMapOverlayTrafficStateOutdated,
  MWMMapOverlayTrafficStateNoData,
  MWMMapOverlayTrafficStateNetworkError,
  MWMMapOverlayTrafficStateExpiredData,
  MWMMapOverlayTrafficStateExpiredApp
} NS_SWIFT_NAME(MapOverlayTrafficState);

typedef NS_ENUM(NSUInteger, MWMMapOverlayTransitState) {
  MWMMapOverlayTransitStateDisabled,
  MWMMapOverlayTransitStateEnabled,
  MWMMapOverlayTransitStateNoData,
} NS_SWIFT_NAME(MapOverlayTransitState);

typedef NS_ENUM(NSUInteger, MWMMapOverlayIsolinesState) {
  MWMMapOverlayIsolinesStateDisabled,
  MWMMapOverlayIsolinesStateEnabled,
  MWMMapOverlayIsolinesStateExpiredData,
  MWMMapOverlayIsolinesStateNoData,
} NS_SWIFT_NAME(MapOverlayTransitState);

typedef NS_ENUM(NSUInteger, MWMMapOverlayOutdoorState) {
  MWMMapOverlayOutdoorStateDisabled,
  MWMMapOverlayOutdoorStateEnabled,
} NS_SWIFT_NAME(MapOverlayOutdoorState);

NS_SWIFT_NAME(MapOverlayManagerObserver)
@protocol MWMMapOverlayManagerObserver <NSObject>

@optional
- (void)onTrafficStateUpdated;
- (void)onTransitStateUpdated;
- (void)onIsoLinesStateUpdated;
- (void)onOutdoorStateUpdated;

@end

NS_SWIFT_NAME(MapOverlayManager)
@interface MWMMapOverlayManager : NSObject

+ (void)addObserver:(id<MWMMapOverlayManagerObserver>)observer;
+ (void)removeObserver:(id<MWMMapOverlayManagerObserver>)observer;

+ (MWMMapOverlayTrafficState)trafficState;
+ (MWMMapOverlayTransitState)transitState;
+ (MWMMapOverlayIsolinesState)isolinesState;
+ (MWMMapOverlayOutdoorState)outdoorState;

+ (BOOL)trafficEnabled;
+ (BOOL)transitEnabled;
+ (BOOL)isoLinesEnabled;
+ (BOOL)isolinesVisible;
+ (BOOL)outdoorEnabled;

+ (void)setTrafficEnabled:(BOOL)enable;
+ (void)setTransitEnabled:(BOOL)enable;
+ (void)setIsoLinesEnabled:(BOOL)enable;
+ (void)setOutdoorEnabled:(BOOL)enable;

@end

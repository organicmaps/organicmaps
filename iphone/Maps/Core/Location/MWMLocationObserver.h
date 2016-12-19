#include "platform/location.hpp"

@protocol MWMLocationObserver<NSObject>

@optional
- (void)onHeadingUpdate:(location::CompassInfo const &)compassinfo;
- (void)onLocationUpdate:(location::GpsInfo const &)gpsInfo;
- (void)onLocationError:(location::TLocationError)locationError;

@end

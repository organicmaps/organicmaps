//#include "platform/location.hpp"

typedef NS_ENUM(NSInteger, MWMLocationStatus) {
  MWMLocationStatusNoError,
  MWMLocationStatusNotSupported,
  MWMLocationStatusDenied,
  MWMLocationStatusGPSIsOff,
  MWMLocationStatusTimeout // Unused on iOS, (only used on Qt)
};

NS_ASSUME_NONNULL_BEGIN

@protocol MWMLocationObserver<NSObject>

@optional
- (void)onHeadingUpdate:(CLHeading *)heading;
- (void)onLocationUpdate:(CLLocation *)location;
- (void)onLocationError:(MWMLocationStatus)locationError;

@end

NS_ASSUME_NONNULL_END

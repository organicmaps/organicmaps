// #include "platform/location.hpp"

typedef NS_ENUM(NSInteger, MWMLocationStatus) {
  MWMLocationStatusNoError,
  MWMLocationStatusNotSupported,
  MWMLocationStatusDenied,
  MWMLocationStatusGPSIsOff,
  MWMLocationStatusTimeout  // Unused on iOS, (only used on Qt)
};

NS_ASSUME_NONNULL_BEGIN

@protocol MWMLocationObserver <NSObject>

@optional
/// Reports the current compass reading on registration and on updates. Nil means the reading is unavailable.
/// A compass reading can have a magnetic heading without a true heading.
- (void)onHeadingUpdate:(nullable CLHeading *)heading;
- (void)onLocationUpdate:(CLLocation *)location;
- (void)onLocationError:(MWMLocationStatus)locationError;

@end

NS_ASSUME_NONNULL_END

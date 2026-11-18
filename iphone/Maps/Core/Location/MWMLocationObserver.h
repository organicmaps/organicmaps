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
/// Replays the current compass reading on registration, then reports valid readings and nil on loss.
/// A compass reading can have a magnetic heading without a true heading.
- (void)onHeadingUpdate:(nullable CLHeading *)heading;
- (void)onLocationUpdate:(CLLocation *)location;
- (void)onLocationError:(MWMLocationStatus)locationError;

@end

NS_ASSUME_NONNULL_END

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// The orders of the enums are the same as in the core (libs/map/nogps).

typedef NS_ENUM(NSInteger, MWMNoGpsPositionSource) {
  MWMNoGpsPositionSourceNone,
  MWMNoGpsPositionSourceGps,
  // Cell towers and Wi-Fi.
  MWMNoGpsPositionSourceNetwork,
  MWMNoGpsPositionSourceManual,
  // Car speed and turns.
  MWMNoGpsPositionSourceInertial,
} NS_SWIFT_NAME(NoGpsPositionSource);

typedef NS_ENUM(NSInteger, MWMNoGpsSourceState) {
  MWMNoGpsSourceStateDisconnected,
  MWMNoGpsSourceStateConnecting,
  MWMNoGpsSourceStateNoAdapter,
  MWMNoGpsSourceStateObdDisabled,
  MWMNoGpsSourceStateObdConnecting,
  MWMNoGpsSourceStateObdError,
  MWMNoGpsSourceStateNoCarData,
  MWMNoGpsSourceStateBoxSleeping,
  MWMNoGpsSourceStateObdSleeping,
  MWMNoGpsSourceStateConnected,
} NS_SWIFT_NAME(NoGpsSourceState);

/// What the phone does to reach the ESP32 sensor box over Bluetooth LE.
typedef NS_ENUM(NSInteger, MWMNoGpsBleState) {
  // Not searched, or the phone has no Bluetooth LE.
  MWMNoGpsBleStateOff,
  MWMNoGpsBleStateNoPermission,
  // Bluetooth is switched off on the phone.
  MWMNoGpsBleStateDisabled,
  MWMNoGpsBleStateSearching,
  // A box is found, but it doesn't accept a new phone now.
  MWMNoGpsBleStatePairingClosed,
  MWMNoGpsBleStateConnecting,
  // The system asks the user for the code of the box.
  MWMNoGpsBleStatePairing,
  // Subscribed to the lines of the box.
  MWMNoGpsBleStateConnected,
} NS_SWIFT_NAME(NoGpsBleState);

typedef NS_ENUM(NSInteger, MWMNoGpsCalibrationState) {
  MWMNoGpsCalibrationStateNone,
  MWMNoGpsCalibrationStateCalibrating,
  MWMNoGpsCalibrationStateDone,
  MWMNoGpsCalibrationStateFailedMoving,
  MWMNoGpsCalibrationStateMountMoved,
} NS_SWIFT_NAME(NoGpsCalibrationState);

typedef NS_ENUM(NSInteger, MWMNoGpsEvent) {
  MWMNoGpsEventManualModeChanged,
  MWMNoGpsEventGpsBack,
  MWMNoGpsEventGpsLost,
  MWMNoGpsEventGpsSpoofed,
  MWMNoGpsEventGpsRestored,
  MWMNoGpsEventRoadLost,
  MWMNoGpsEventTurnsReversed,
  MWMNoGpsEventMotionSourceStopped,
  MWMNoGpsEventMarkNoRoad,
} NS_SWIFT_NAME(NoGpsEvent);

/// Posted on the main queue, the MWMNoGpsEvent is in the userInfo by MWMNoGpsEventKey.
extern NSNotificationName const MWMNoGpsEventNotification NS_SWIFT_NAME(NoGps.eventNotification);
extern NSString * const MWMNoGpsEventKey NS_SWIFT_NAME(NoGps.eventKey);

/// Everything the UI shows about the navigation without GPS, taken at once.
NS_SWIFT_NAME(NoGpsStatus)
@interface MWMNoGpsStatus : NSObject

@property(nonatomic, readonly) MWMNoGpsPositionSource source;
// Of the shown position.
@property(nonatomic, readonly) double accuracyM;
@property(nonatomic, readonly) BOOL manualMode;
// Since the user has set the position in the manual mode.
@property(nonatomic, readonly) NSTimeInterval manualAge;
@property(nonatomic, readonly) BOOL gpsSpoofed;
@property(nonatomic, readonly) BOOL gpsDisabled;
// GPS works now, also in the manual mode where it is not used. Negative if it doesn't.
@property(nonatomic, readonly) double workingGpsAccuracyM;
@property(nonatomic, readonly) double workingNetworkAccuracyM;
// The position is not from GPS: the user corrects it with the buttons.
@property(nonatomic, readonly) BOOL movedByHand;
// The position stands at a crossing and is moved that way after the car passes it.
@property(nonatomic, readonly) BOOL shiftForwardBlocked;
@property(nonatomic, readonly) BOOL shiftBackBlocked;
@property(nonatomic, readonly) NSInteger shiftStepM;
@property(nonatomic, readonly) BOOL shiftButtonsShown;

@property(nonatomic, readonly) BOOL inertialEnabled;
// The rest is known while the inertial navigation works.
@property(nonatomic, readonly) BOOL inertialStarted;
@property(nonatomic, readonly) MWMNoGpsSourceState sourceState;
@property(nonatomic, readonly) MWMNoGpsBleState bleState;
@property(nonatomic, readonly) NSString * deviceName;
// Negative if unknown.
@property(nonatomic, readonly) NSInteger speedKmh;
@property(nonatomic, readonly) BOOL hasCarInfo;
// Negative if unknown, 0 if stopped, 1 if running.
@property(nonatomic, readonly) NSInteger engineRunning;
// The car data, negative if unknown.
@property(nonatomic, readonly) NSInteger rpm;
@property(nonatomic, readonly) NSInteger boxMillivolts;
@property(nonatomic, readonly) NSInteger elmMillivolts;
@property(nonatomic, readonly) NSInteger ecuMillivolts;
@property(nonatomic, readonly) BOOL voltageMismatch;
@property(nonatomic, readonly) double speedScale;
@property(nonatomic, readonly) NSInteger speedTableRanges;
@property(nonatomic, readonly) double speedLagSec;
@property(nonatomic, readonly) BOOL speedLagMeasured;
@property(nonatomic, readonly) MWMNoGpsCalibrationState calibration;
@property(nonatomic, readonly) NSInteger calibrationProgress;
@property(nonatomic, readonly) BOOL hasInertialPosition;
@property(nonatomic, readonly) BOOL paused;
/// The pause is set by the navigation itself: the car maneuvers off the roads, its mark waits on the road.
@property(nonatomic, readonly) BOOL pausedByItself;

@end

/// Navigation without GPS, done in the core: it chooses the position to show from GPS, the marks of the user and the
/// inertial navigation by the ESP32 sensor box. Everything is called on the main queue.
NS_SWIFT_NAME(NoGps)
@interface MWMNoGps : NSObject

/// A position from CoreLocation. The chosen position comes back to MWMLocationManager.
+ (void)onLocation:(CLLocation *)location;

+ (void)start;
+ (void)stop;

+ (MWMNoGpsStatus *)status;

+ (BOOL)isManualMode;
+ (void)setManualMode:(BOOL)enabled;
/// Moves the position along the route or the road.
/// @return the distance it was moved by, 0 if it can not be moved.
+ (double)shiftPosition:(double)distanceM;
+ (void)reverseDirection;
+ (void)togglePause;
+ (void)setGpsDisabled:(BOOL)disabled;
+ (void)setInertialNavigationEnabled:(BOOL)enabled;
+ (void)calibrate;
+ (void)clearSpeedCalibration;
+ (void)cycleShiftStep;
+ (void)setShiftButtonsShown:(BOOL)shown;

/// The bytes of the ESP32 sensor box and the state of the connection to it.
+ (void)onEsp32BleBytes:(NSData *)data;
+ (void)onEsp32BleState:(MWMNoGpsBleState)state;

@end

NS_ASSUME_NONNULL_END

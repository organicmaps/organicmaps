#import "MWMNoGps.h"
#import "MWMLocationManager.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/engine.hpp"

#include "base/logging.hpp"

#include <memory>

NSNotificationName const MWMNoGpsEventNotification = @"MWMNoGpsEventNotification";
NSString * const MWMNoGpsEventKey = @"event";

namespace
{
NSData * ToData(std::string const & bytes)
{
  return [NSData dataWithBytes:bytes.data() length:bytes.size()];
}

/// The iOS part of the navigation without GPS: the car movement comes from the ESP32 sensor box only, over Bluetooth
/// LE. The phone with an ELM327 adapter is not offered: iOS has no Bluetooth SPP for the adapters.
class NoGpsDelegate : public nogps::Delegate
{
public:
  void StartMotionSensors(bool) override {}
  void StopMotionSensors() override {}

  void Elm327Connect(std::string const &) override {}
  void Elm327Write(std::string const &) override {}
  void Elm327Close() override {}

  void Esp32BleOpen() override { [m_esp32 open]; }
  void Esp32BleSend(std::string const & line) override { [m_esp32 send:ToData(line)]; }
  void Esp32BleSendFirmware(std::string const & piece) override { [m_esp32 sendFirmware:ToData(piece)]; }
  void Esp32BleClose() override { [m_esp32 close]; }

  bool IsBoxPairing() const { return m_esp32.isPairing; }

  void OnPosition(nogps::Fix const & fix) override
  {
    CLLocationCoordinate2D const coordinate = CLLocationCoordinate2DMake(fix.m_position.m_lat, fix.m_position.m_lon);
    CLLocation * location =
        [[CLLocation alloc] initWithCoordinate:coordinate
                                      altitude:fix.m_altitudeM.value_or(0)
                            horizontalAccuracy:fix.m_accuracyM
                              verticalAccuracy:fix.m_altitudeM ? fix.m_altitudeAccuracyM.value_or(0) : -1
                                        course:fix.m_bearingDeg.value_or(-1)
                                         speed:fix.m_speedMps.value_or(-1)
                                     timestamp:[NSDate dateWithTimeIntervalSince1970:fix.m_unixTimeMs / 1000.0]];
    bool const fromGps = fix.m_provider == nogps::Provider::Gps || fix.m_provider == nogps::Provider::Fused;
    [MWMLocationManager onNoGpsPosition:location fromGps:fromGps];
  }

  void OnEvent(nogps::Event event) override
  {
    [NSNotificationCenter.defaultCenter postNotificationName:MWMNoGpsEventNotification
                                                      object:nil
                                                    userInfo:@{
                                                      MWMNoGpsEventKey: @(static_cast<NSInteger>(event))
                                                    }];
  }

private:
  Esp32BleTransport * m_esp32 = [[Esp32BleTransport alloc] init];
};

NoGpsDelegate & Delegate()
{
  static NoGpsDelegate delegate;
  return delegate;
}

nogps::Engine & Engine()
{
  static nogps::Engine * engine = nullptr;
  if (!engine)
  {
    engine = &GetFramework().CreateNoGps(Delegate());
    // The box is the only source of the car movement on iOS.
    if (!engine->GetStatus().m_esp32Source)
      engine->SetEsp32Source(true);
  }
  return *engine;
}

std::optional<double> NonNegative(double value)
{
  if (value < 0)
    return {};
  return value;
}
}  // namespace

@interface MWMNoGpsStatus ()

- (instancetype)initWithStatus:(nogps::Status const &)status;

@end

@implementation MWMNoGpsStatus

- (instancetype)initWithStatus:(nogps::Status const &)status
{
  self = [super init];
  if (self)
  {
    _source = static_cast<MWMNoGpsPositionSource>(status.m_source);
    _accuracyM = status.m_accuracyM;
    _manualMode = status.m_manualMode;
    _manualAge = status.m_manualAgeMs / 1000.0;
    _gpsSpoofed = status.m_gpsSpoofed;
    _gpsDisabled = status.m_gpsDisabled;
    _workingGpsAccuracyM = status.m_workingGpsAccuracyM.value_or(-1);
    _workingNetworkAccuracyM = status.m_workingNetworkAccuracyM.value_or(-1);
    _movedByHand = status.m_movedByHand;
    _shiftForwardBlocked = status.m_shiftForwardBlocked;
    _shiftBackBlocked = status.m_shiftBackBlocked;
    _shiftStepM = status.m_shiftStepM;
    _shiftButtonsShown = status.m_shiftButtonsShown;
    _inertialEnabled = status.m_inertialEnabled;
    _inertialStarted = status.m_inertialStarted;
    _sourceState = static_cast<MWMNoGpsSourceState>(status.m_sourceState);
    _bleState = static_cast<MWMNoGpsBleState>(status.m_bleState);
    _deviceName = @(status.m_deviceName.c_str());
    _speedKmh = status.m_speedKmh;
    _hasCarInfo = status.m_carInfo.has_value();
    auto const car = status.m_carInfo.value_or(nogps::CarInfo());
    _engineRunning = car.m_engineRunning ? static_cast<NSInteger>(*car.m_engineRunning) : -1;
    _rpm = car.m_rpm;
    _boxMillivolts = car.m_boxMillivolts;
    _elmMillivolts = car.m_elmMillivolts;
    _ecuMillivolts = car.m_ecuMillivolts;
    _voltageMismatch = car.m_voltageMismatch;
    _speedScale = status.m_speedScale;
    _speedTableRanges = status.m_speedTableRanges;
    _speedLagSec = status.m_speedLagSec;
    _speedLagMeasured = status.m_speedLagMeasured;
    _calibration = static_cast<MWMNoGpsCalibrationState>(status.m_calibration);
    _calibrationProgress = status.m_calibrationProgress;
    _hasInertialPosition = status.m_hasInertialPosition;
    _paused = status.m_paused;
    _pausedByItself = status.m_pausedByItself;
  }
  return self;
}

@end

@implementation MWMNoGps

+ (void)onLocation:(CLLocation *)location
{
  static nogps::SystemClock const clock;
  NSTimeInterval const age = -location.timestamp.timeIntervalSinceNow;
  nogps::Fix fix;
  // CoreLocation mixes GPS with Wi-Fi and cell towers, the core tells them apart by the accuracy.
  fix.m_provider = nogps::Provider::Fused;
  fix.m_position = {location.coordinate.latitude, location.coordinate.longitude};
  fix.m_accuracyM = location.horizontalAccuracy;
  fix.m_bearingDeg = NonNegative(location.course);
  fix.m_bearingAccuracyDeg = NonNegative(location.courseAccuracy);
  fix.m_speedMps = NonNegative(location.speed);
  if (location.verticalAccuracy >= 0)
  {
    fix.m_altitudeM = location.altitude;
    fix.m_altitudeAccuracyM = location.verticalAccuracy;
  }
  fix.m_timeMs = clock.NowMs() - static_cast<int64_t>(age * 1000);
  fix.m_unixTimeMs = static_cast<int64_t>(location.timestamp.timeIntervalSince1970 * 1000);
  Engine().OnFix(fix);
}

+ (void)start
{
  Engine().Start();
}

+ (void)stop
{
  Engine().Stop();
}

+ (MWMNoGpsStatus *)status
{
  auto status = Engine().GetStatus();
  // The core doesn't know about the pairing: iOS doesn't tell it, see Esp32BleTransport.isPairing.
  if (Delegate().IsBoxPairing())
    status.m_bleState = nogps::BleState::Pairing;
  return [[MWMNoGpsStatus alloc] initWithStatus:status];
}

+ (BOOL)isManualMode
{
  return Engine().IsManualMode();
}

+ (void)setManualMode:(BOOL)enabled
{
  Engine().SetManualMode(enabled);
}

+ (double)shiftPosition:(double)distanceM
{
  return Engine().ShiftPosition(distanceM);
}

+ (void)reverseDirection
{
  Engine().ReverseDirection();
}

+ (void)togglePause
{
  Engine().TogglePause();
}

+ (void)setGpsDisabled:(BOOL)disabled
{
  Engine().SetGpsDisabled(disabled);
}

+ (void)setInertialNavigationEnabled:(BOOL)enabled
{
  Engine().SetInertialNavigationEnabled(enabled);
}

+ (void)calibrate
{
  Engine().Calibrate();
}

+ (void)clearSpeedCalibration
{
  Engine().ClearSpeedCalibration();
}

+ (void)cycleShiftStep
{
  Engine().CycleShiftStep();
}

+ (void)setShiftButtonsShown:(BOOL)shown
{
  Engine().SetShiftButtonsShown(shown);
}

+ (void)onEsp32BleBytes:(NSData *)data
{
  Engine().OnEsp32BleBytes({static_cast<char const *>(data.bytes), data.length});
}

+ (void)onEsp32BleState:(MWMNoGpsBleState)state
{
  Engine().OnEsp32BleState(static_cast<nogps::BleState>(state));
}

@end

#pragma once

#include "map/nogps/delegate.hpp"
#include "map/nogps/esp32_update.hpp"
#include "map/nogps/gps_return_detector.hpp"
#include "map/nogps/gps_spoofing_detector.hpp"
#include "map/nogps/gyro_calibrator.hpp"
#include "map/nogps/inertial_navigator.hpp"
#include "map/nogps/motion_source.hpp"
#include "map/nogps/scheduler.hpp"

#include "geometry/latlon.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nogps
{
class Clock;
class MapApi;
class Storage;

enum class PositionSource
{
  None,
  Gps,
  // Cell towers and Wi-Fi.
  Network,
  Manual,
  // Car speed and turns.
  Inertial,
};

std::string DebugPrint(PositionSource source);

/// Everything the UI shows about the navigation without GPS, taken at once.
struct Status
{
  // Where the shown position comes from and its accuracy.
  PositionSource m_source = PositionSource::None;
  double m_accuracyM = 0;
  bool m_manualMode = false;
  // The next tap on the map shows where the car is: the button for it is pressed.
  bool m_markByTap = false;
  // Since the user has set the position in the manual mode.
  int64_t m_manualAgeMs = 0;
  bool m_gpsSpoofed = false;
  bool m_gpsDisabled = false;
  // GPS works now, also in the manual mode where it is not used.
  std::optional<double> m_workingGpsAccuracyM;
  std::optional<double> m_workingNetworkAccuracyM;
  // The position is not from GPS: the user corrects it with the buttons.
  bool m_movedByHand = false;
  // The position stands at a crossing and is moved that way after the car passes it.
  bool m_shiftForwardBlocked = false;
  bool m_shiftBackBlocked = false;
  int m_shiftStepM = 0;
  bool m_shiftButtonsShown = false;

  bool m_inertialEnabled = false;
  bool m_esp32Source = false;
  std::string m_elm327Address;
  // The rest is known while the inertial navigation works.
  bool m_inertialStarted = false;
  SourceState m_sourceState = SourceState::Disconnected;
  // What the platform does to reach the ESP32 sensor box over Bluetooth.
  BleState m_bleState = BleState::Off;
  std::string m_deviceName;
  // -1 if unknown.
  int m_speedKmh = -1;
  std::optional<CarInfo> m_carInfo;
  double m_speedScale = 1;
  int m_speedTableRanges = 0;
  // The rough table learned from the roads is used while GPS has measured no range.
  int m_roughSpeedTableRanges = 0;
  double m_speedLagSec = 0;
  bool m_speedLagMeasured = false;
  CalibrationState m_calibration = CalibrationState::None;
  int m_calibrationProgress = 0;
  bool m_hasInertialPosition = false;
  // The mark waits for the user to put it on a road: the road is lost. It is to be noticed at once.
  bool m_roadLost = false;
  // How many other roads the car may be on after a turn that fitted several, see InertialNavigator::kMaxMarks: the
  // user can switch the mark to them with SwitchMark().
  int m_otherMarks = 0;

  // The firmware of the ESP32 sensor box, empty until the box tells it.
  std::string m_boxFirmware;
  // The firmware for this box that comes with the application, if the box can take it now: it is connected
  // over Bluetooth. Empty otherwise.
  std::string m_bundledFirmware;
  Esp32Update::State m_firmwareUpdate = Esp32Update::State::None;
  int m_firmwareUpdateProgress = 0;
  // Why the update has failed, see Esp32Update::GetError().
  std::string m_firmwareUpdateError;
  // The sensor box has been powered on since the user calibrated it: it may stand in another way now.
  bool m_boxCalibrationAdvised = false;
};

/// Navigation without GPS: it chooses the position to show from GPS, cell towers, the marks of the user and the
/// inertial navigation. GPS jammed or spoofed is detected and the manual mode is turned on by itself, and left when
/// GPS is trusted again. Runs on the GUI thread.
class Engine : private InertialNavigator::Listener
{
public:
  // The manual position is re-sent to the core periodically, otherwise the location is treated as lost.
  static int64_t constexpr kManualRepeatIntervalMs = 5000;
  // Taps on the map are not precise, so the core matches a manual position to the route within this radius.
  static double constexpr kManualAccuracyM = 50;
  // The car stands on a road, not between houses, but a farther road is not the road the user means.
  static double constexpr kManualSnapRadiusM = 30;
  // A road going another way is a crossing road: the car is on it, but it does not drive along it.
  static double constexpr kMaxRoadBearingDiffDeg = 45;
  // A point of the route closer than this to a road is on it, and goes along it within the angle.
  static double constexpr kRoadCheckRadiusM = 3;
  static double constexpr kMaxAlongRoadDiffDeg = 30;
  // Closer to the previous position, the side the car has turned to is not known.
  static double constexpr kMinTurnDistanceM = 5;
  // The position stopped at a turn is not moved further: the car can turn to another street there.
  // It is moved again after the car has really left the turn by this distance.
  static double constexpr kShiftBlockRadiusM = 10;
  static double constexpr kMinShiftM = 0.5;
  // A position without updates for longer than this is shown as lost.
  static int64_t constexpr kPositionStaleMs = 30'000;
  // The fused provider mixes GPS with cell towers and Wi-Fi: less accurate positions are from the network.
  static double constexpr kFusedSatelliteMaxAccuracyM = 30;
  // A position closer to the route than this is considered on the route even if it is very accurate.
  static double constexpr kMinOffRouteDistanceM = 50;
  // A position set by the user lies on the axis of the route if the car is on the route. The core keeps the
  // car on the route ahead of a farther one: it moves along the route forward only, and the car would be
  // shown ahead of the mark, e.g. after a mark behind the previous one. The route is rebuilt from it then.
  static double constexpr kManualOffRouteDistanceM = 10;
  // A car driving along a road farther than this from the route for this long has left the route, see
  // LeaveRouteIfBesideIt(). Two roads side by side are farther apart, a mark on the road of the route is
  // nearer: it is on the line of the route.
  static double constexpr kBesideRouteM = 15;
  static int64_t constexpr kBesideRouteMs = 2000;
  // A mark this near to the route is on its road.
  static double constexpr kOnRouteRoadM = 3;
  // Trusted GPS positions are preferred over the inertial ones while they are this fresh. GPS gives a
  // position every second, so it is lost after a missed one.
  static int64_t constexpr kGpsFreshMs = 2000;
  // Without the inertial navigation a longer pause of GPS is waited for while navigating: the manual mode
  // needs a mark from the user.
  static int64_t constexpr kGpsLostMs = 5000;
  // The first GPS position after the start takes longer: the satellites are searched.
  static int64_t constexpr kGpsFirstFixMs = 15'000;
  // GPS gives a position every second, and the map moves the mark to a new one in 0.2 s: it would jump and stand.
  // Between GPS positions the mark is moved as often as the inertial navigation gives its positions.
  static int64_t constexpr kGpsFillIntervalMs = InertialNavigator::kOutputIntervalMs;
  // Without the inertial navigation the last GPS position is moved along its bearing for one missed position at
  // most: a straight line is 5 m aside of a curve of 100 m after 2 s at 60 km/h.
  static int64_t constexpr kMaxGpsExtrapolationMs = 2000;
  // GPS farther than this from the inertial position following it is spoofed or broken: the inertial
  // position is calculated for a second only since the previous GPS position.
  static double constexpr kMaxGpsInertialDiffM = 30;
  // A less accurate GPS position is jammed, the inertial navigation is better.
  static double constexpr kGpsMaxAccuracyM = 30;
  // GPS farther from the roads than this plus its accuracy is led away by a spoofer, after several positions
  // in a row: a single one can be noise.
  static double constexpr kMaxGpsOffRoadM = 20;
  static int constexpr kOffRoadGpsCount = 3;
  // The car speed errors are a few percent, they are measured by accurate GPS only.
  static double constexpr kSpeedTableGpsMaxAccuracyM = 10;
  static double constexpr kOffRoadMinSpeedMps = 5;
  // The inertial navigation is considered active while its positions are used this recently.
  static int64_t constexpr kInertialActiveMs = 1000;
  // Old positions (e.g. the last known one after a provider restart) tell nothing about the car now.
  static int64_t constexpr kGpsMaxAgeMs = 5000;
  // The state is written to the log every second, so a drive can be analysed afterwards.
  static int64_t constexpr kTripLogIntervalMs = 1000;
  // GPS gives a position every second, a longer pause means it is lost.
  static int64_t constexpr kGpsStatusMaxAgeMs = 10'000;
  // GPS works when it gives a position every second, every 3 while the map does not follow it. The fused provider
  // of Android repeats the cell tower and Wi-Fi positions as accurate ones every 6: without GPS in the phone on
  // October 14 they were 3 m accurate, and the user was told that GPS was back.
  static int64_t constexpr kGpsStatusMaxGapMs = 4000;
  // Cell tower positions come once in several seconds.
  static int64_t constexpr kNetworkStatusMaxAgeMs = 60'000;
  // The trusted position is saved not more often than this.
  static int64_t constexpr kSaveTrustedIntervalMs = 30'000;
  // The place of a driving car is saved not more often than this, of a standing one at once: the car is left where
  // it stands. A car that has moved less than this in a second stands.
  static int64_t constexpr kSaveCarIntervalMs = 10'000;
  static double constexpr kCarStandsM = 1;
  // The stopped car speed is told repeatedly: a message is easily missed while driving.
  static int64_t constexpr kMotionSourceWarningIntervalMs = 30'000;
  // The gyroscope not calibrated is told about when the car drives this fast for this long: the sensor box needs
  // a couple of seconds after it wakes up.
  static int constexpr kNotCalibratedSpeedKmh = 10;
  static int64_t constexpr kNotCalibratedDelayMs = 5000;
  // A new firmware is offered when the sensor box has been heard for this long: it tells the car speed by then.
  static int64_t constexpr kFirmwareOfferDelayMs = 5000;
  // Two power-ons of the sensor box are the same one if their times differ less than this and this part of the
  // time the box has worked since: it sleeps most of its time, and its clock is not exact then.
  static int64_t constexpr kSamePowerOnMs = 30'000;
  static double constexpr kBoxClockError = 0.1;
  // Meters the position is moved by with the buttons, the user chooses one of them.
  static int constexpr kShiftStepsM[] = {10, 20, 50, 100};

  Engine(Delegate & delegate, MapApi & map, Storage & storage, Clock const & clock, Scheduler & scheduler);
  ~Engine() override;

  /// Starts and stops with the location updates of the platform.
  void Start();
  void Stop();
  bool IsActive() const { return m_active; }

  /// A position from the platform: GPS, cell towers or both mixed.
  void OnFix(Fix const & fix);
  /// The compass of the phone, degrees: it is only written to the log of a drive, to see if it can tell the way the
  /// car looks when GPS has not told it.
  void OnCompass(double headingDeg, double accuracyDeg);
  /// The last known network position at the start: GPS is verified against it before a fresh one comes.
  void OnLastKnownNetworkFix(Fix const & fix);

  // The sensors of the phone, Android axes and units: rad/s counter-clockwise, m/s² with the reaction to gravity.
  void OnGyro(int64_t timestampNs, double x, double y, double z);
  void OnAccel(int64_t timestampNs, double x, double y, double z);
  void OnElm327Connected();
  void OnElm327Bytes(std::string_view data);
  void OnElm327Closed(std::string const & reason);
  // The ESP32 sensor box over Bluetooth LE, see Delegate::Esp32BleOpen().
  void OnEsp32BleBytes(std::string_view bytes);
  void OnEsp32BleState(BleState state);

  bool IsManualMode() const { return m_manualMode; }
  void SetManualMode(bool enabled);
  /// Puts the car on the road at the position, e.g. tapped on the map. Works only in the manual mode.
  /// \returns false if there is no road at the position: the car is always on a road.
  bool SetManualLocation(ms::LatLon const & position);
  /// The next tap on the map shows where the car is, see PlaceMarkByTap(): the user has pressed the button for it.
  void SetMarkByTap(bool armed);
  bool IsMarkByTap() const { return m_markByTap; }
  /// The user has tapped the map to show where the car is. The car is followed from there without GPS, in the
  /// manual mode, which is left by itself when GPS is trusted again. Ends SetMarkByTap().
  /// \returns false if there is no road at the position.
  bool PlaceMarkByTap(ms::LatLon const & position);
  /// Moves the position along the route, or along the road if there is no route, e.g. when the calculated
  /// position lags behind the car. The position never leaves the road and stops at the closest crossing.
  /// \param distanceM meters to move forward, negative to move back.
  /// \returns the distance the position was moved by, 0 if it can not be moved.
  double ShiftPosition(double distanceM);
  /// \returns true if the position has stopped at a turn and must not be moved that way until the car
  /// leaves the turn: the car can take another street there.
  /// \param forward true for the direction of the movement, false for the opposite one.
  bool IsShiftBlocked(bool forward);
  /// Turns the car around on its road: the car looks one of the two ways of the road, the user chooses the
  /// other one.
  void ReverseDirection();
  /// Shows the car on the next of the other roads it may be on, see Status::m_otherMarks, and rebuilds the route
  /// from there. The road it was shown on stays one of them.
  void SwitchMark();
  /// Ignores GPS as if it were jammed, e.g. to test the navigation without GPS where GPS works. It is not
  /// kept after a restart, so it is not left on by mistake.
  void SetGpsDisabled(bool disabled);

  void SetInertialNavigationEnabled(bool enabled);
  bool IsInertialNavigationEnabled() const;
  void SetElm327Address(std::string const & address);
  /// Chooses where the car movement comes from: the ESP32 sensor box or the phone with an ELM327 adapter.
  void SetEsp32Source(bool esp32);
  void Calibrate();
  /// Forgets the car speed errors, e.g. after new tyres.
  void ClearSpeedCalibration();

  /// The firmwares of the ESP32 sensor box that come with the application: the box should work with the one
  /// for it, the application is tested with it.
  void SetFirmwares(std::vector<Firmware> firmwares);
  /// Sends the firmware of Status::m_bundledFirmware to the box.
  void StartFirmwareUpdate();
  void CancelFirmwareUpdate();

  /// Switches to the next of kShiftStepsM.
  void CycleShiftStep();
  /// The buttons moving the position along the road are shown: they calibrate the speed of the car.
  void SetShiftButtonsShown(bool shown);

  /// \returns true if the position arrow must show the car direction instead of the compass.
  bool IsCarHeadingShown() const;

  Status GetStatus();
  /// The whole state of the navigation in a line. It is written to the log every second, the same on every
  /// platform, so a drive can be analysed afterwards.
  std::string GetTripLine();

private:
  // InertialNavigator::Listener overrides:
  void OnInertialLocation(Fix const & fix) override;
  void OnRoadLost() override;
  void OnTurnsReversed() override;
  void OnMarkSwitched() override;

  static PositionSource SourceOf(Fix const & fix);
  int64_t AgeMs(Fix const & fix) const;
  // Returns the time since the moment, the longest one if it hasn't happened.
  int64_t MsSince(std::optional<int64_t> timeMs) const;
  int64_t GetGpsSilenceMs() const;
  bool IsGpsBack(Fix const & gps);
  bool IsSpeedTableGps(Fix const & gps);
  bool ContradictsInertial(Fix const & gps) const;
  bool IsOffRoadWhileDriving(Fix const & gps);
  bool IsNearRoad(Fix const & gps);
  bool IsAlongRoad(ms::LatLon const & position, double bearingDeg);
  std::optional<RoadPoint> SnapMarkToRoad(ms::LatLon const & position, std::optional<double> bearingDeg, double radiusM,
                                          std::optional<Fix> const & turnedFrom);
  /// The road the car is put on by a mark at the position, nothing if there is none.
  std::optional<RoadPoint> FindMarkRoad(ms::LatLon const & position);
  void PutMark(ms::LatLon const & position, RoadPoint const & road);
  void EnterManualModeByItself(std::string const & reason);
  void CheckGpsLost();
  void CheckMotionSourceStopped();
  void CheckNotCalibrated();
  void CheckFirmwareUpdate();
  void CheckBoxCalibration();
  /// \returns the firmware for the connected sensor box, nullptr if there is none or the box can't take it now.
  Firmware const * FindBoxFirmware();
  void OnGpsSpoofingChanged(bool spoofed);
  void RebuildRouteIfOffRoute(Fix const & fix);
  void LeaveRouteIfBesideIt(Fix const & inertial);
  void SaveTrustedPosition();
  void RestoreTrustedPosition();
  /// \param now true to save a driving car too, e.g. when the location updates stop.
  void SaveCarPosition(bool now);
  /// Shows the car where it was left before the app was closed: it is not known otherwise until GPS or the user
  /// tells it.
  void RestoreCarPosition();
  void StartInertialNavigation();
  bool IsInertialActive() const;
  std::optional<double> GetCarBearing() const;
  void ShowCarHeading();
  void ShowOffRoadCar();
  void NotifyLocationUpdated();
  /// Shows the car between GPS positions, see kGpsFillIntervalMs: by the inertial navigation, which continues from
  /// every good GPS position. GPS stays the position of the engine.
  void ShowBetweenGpsPositions(Fix const & inertial);
  /// The same without the inertial navigation: moves the last GPS position along its bearing.
  void ExtrapolateGps();
  /// \returns the time of a position shown after the GPS one by the clock of GPS: the clock of the phone differs from
  /// it, and the core moves the mark by the times of the positions, see extrapolation::Extrapolator.
  int64_t UnixTimeAfterGps(int64_t timeMs) const;
  void ApplyManualLocation();
  void BlockShift(bool forward, ms::LatLon const & at);
  std::optional<Fix> GetWorkingGps() const;
  std::optional<Fix> GetWorkingNetwork() const;
  PositionSource GetPositionSource() const;
  void LogTrip();
  void Notify(Event event);

  Delegate & m_delegate;
  MapApi & m_map;
  Storage & m_storage;
  Clock const & m_clock;
  Scheduler & m_scheduler;

  bool m_active = false;
  // The position passed to the platform the last time.
  std::optional<Fix> m_savedLocation;
  // In the manual mode GPS is ignored (it is jammed or spoofed) and the position is set by the user.
  bool m_manualMode = false;
  bool m_gpsDisabled = false;
  std::optional<Fix> m_manualLocation;
  // The next tap on the map shows where the car is.
  bool m_markByTap = false;
  // The last position set by the user.
  int64_t m_manualSetTimeMs = 0;
  // The last GPS position good enough to be used. It is kept in the manual mode too, where GPS is not used,
  // to tell the user that GPS works again.
  std::optional<Fix> m_lastGoodGps;
  // Since the good GPS position before it.
  int64_t m_lastGoodGpsGapMs = std::numeric_limits<int64_t>::max();
  // The last positions as the platform gave them, also not trusted or ignored: the log of a drive has them to
  // compare the shown position with. The satellites alone, and mixed with Wi-Fi and cell towers: iOS has only
  // the mix, Android may have both.
  std::optional<Fix> m_rawGps;
  std::optional<Fix> m_rawFused;
  struct Compass
  {
    double m_headingDeg = 0;
    double m_accuracyDeg = 0;
    int64_t m_timeMs = 0;
  };
  std::optional<Compass> m_compass;
  // After the manual mode the next real position must replace the manual one, even if it is less accurate.
  bool m_acceptNextLocation = false;
  // The source of the last position passed to the core, used to check the route when the source changes.
  PositionSource m_lastPositionSource = PositionSource::None;
  // The turns the position has stopped at, nothing if the position can be moved that way.
  std::optional<ms::LatLon> m_shiftStopForward;
  std::optional<ms::LatLon> m_shiftStopBack;

  std::unique_ptr<InertialNavigator> m_inertial;
  std::optional<int64_t> m_lastTrustedGpsMs;
  int64_t m_savedTrustedTimeMs = 0;
  // The manual mode is on because there was no position at all, the user hasn't placed the car yet.
  bool m_manualForNoPosition = false;
  // The manual mode shows the car where it was left while GPS was trusted, until the first good GPS position.
  bool m_manualUntilGps = false;
  // The place of the car last saved, without the time, and when; where the car was a second ago.
  std::string m_savedCar;
  std::optional<int64_t> m_carSavedMs;
  std::optional<ms::LatLon> m_carSecondAgo;
  // When the manual mode was left: GPS was ignored in it, so it is waited for anew.
  std::optional<int64_t> m_manualLeftMs;
  // GPS positions off the roads in a row.
  int m_offRoadGpsCount = 0;
  // The start of the location updates.
  int64_t m_startTimeMs = 0;
  std::optional<int64_t> m_lastInertialUsedMs;
  // Since when the car has driven along a road beside the route.
  std::optional<int64_t> m_besideRouteSinceMs;
  // The mark has been on the road of the route since the route was left by a car beside it: a route that
  // does not start from the road of the car is not rebuilt again and again.
  bool m_wasOnRouteRoad = true;
  // The car maneuvering off the roads as the map shows it now, see ShowOffRoadCar().
  std::optional<ms::LatLon> m_shownOffRoadCar;
  // When the last position of the platform was shown: the positions between GPS ones go on from it since then.
  int64_t m_fixShownMs = 0;
  // Positions shown between GPS positions since the last line of the trip log.
  int m_positionsBetweenGps = 0;
  // The motion source of the inertial navigation gave the car speed, the user is told when it stops.
  bool m_motionSourceWorked = false;
  std::optional<int64_t> m_motionSourceWarnedMs;
  // Since when the car drives with the gyroscope not calibrated, and when the user was told about it.
  std::optional<int64_t> m_notCalibratedSinceMs;
  std::optional<int64_t> m_notCalibratedWarnedMs;

  std::vector<Firmware> m_firmwares;
  Esp32Update::State m_firmwareUpdateState = Esp32Update::State::None;
  // The firmware of the box and of the application the user was asked about, not to ask again.
  std::string m_offeredFirmware;
  // Since when the box is known, and if the car has driven since: the update is offered before a trip, not at
  // a traffic light.
  std::optional<int64_t> m_boxKnownSinceMs;
  bool m_drivenWithBox = false;
  // The last calibration of the box by the user that is taken into account.
  std::optional<int64_t> m_boxCalibratedMs;
  // The box has been powered on, the user is to be told when the car stands.
  bool m_boxCalibrationAdvicePending = false;

  GpsSpoofingDetector m_spoofingDetector;
  GpsReturnDetector m_gpsReturnDetector;
  // Tells when GPS has worked long enough to measure the car speed errors by it.
  GpsReturnDetector m_speedTableGpsDetector;
  std::optional<Fix> m_networkLocation;

  Timer m_manualRepeatTimer;
  Timer m_tripLogTimer;
  Timer m_gpsExtrapolationTimer;

  ThreadChecker m_threadChecker;
};
}  // namespace nogps

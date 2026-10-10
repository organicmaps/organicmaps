#pragma once

#include "map/nogps/accel_log.hpp"
#include "map/nogps/accel_speed.hpp"
#include "map/nogps/bend_matcher.hpp"
#include "map/nogps/dead_reckoning.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/esp32_source.hpp"
#include "map/nogps/gyro_calibrator.hpp"
#include "map/nogps/motion_source.hpp"
#include "map/nogps/speed_lag.hpp"
#include "map/nogps/speed_scale.hpp"
#include "map/nogps/speed_table.hpp"
#include "map/nogps/turn_sign_checker.hpp"

#include "geometry/latlon.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace nogps
{
class Clock;
class Elm327Session;
class PhoneMotion;
class Roads;
class Scheduler;
class Storage;

/// Inertial navigation: the car speed and turns come from a MotionSource, the phone fixed on the dashboard with
/// an ELM327 OBD-II adapter or the ESP32 sensor box.
/// It needs a reference position and heading (from GPS while it was trusted, manual marks or the user) and
/// a calibrated gyroscope.
class InertialNavigator : private MotionSource::Listener
{
public:
  enum class HeadingSource
  {
    None,
    Gps,
    // The direction of the road the car is on, the user has chosen one of its two ways.
    Road,
  };

  // What the last look for the road under the car has ended with, for the trip log.
  enum class SnapResult
  {
    // The car was not looked for yet.
    None,
    OnRoad,
    // No road around.
    NoRoad,
    // The road is farther than kSnapRadiusM.
    Far,
    // The road goes another way than the car looks.
    Angle,
    // A maneuvering car is not on a road far aside until it has driven along it.
    Unconfirmed,
  };

  class Listener
  {
  public:
    virtual ~Listener() = default;
    virtual void OnInertialLocation(Fix const & fix) = 0;
    /// The car has left the roads, it is stopped where it was on a road the last time.
    virtual void OnRoadLost() = 0;
    /// The gyroscope turns the car the other way than GPS: the axes of its driver are wrong.
    virtual void OnTurnsReversed() = 0;
  };

  // Without fresh speed the position can't be calculated.
  static int64_t constexpr kSpeedStaleMs = 2000;
  static int64_t constexpr kOutputIntervalMs = 200;
  static double constexpr kMovingSpeedMps = 0.5;
  // GPS bearing is noisy at low speeds, unless GPS tells it is accurate: it is so from a walking speed on.
  static double constexpr kMinGpsHeadingSpeedMps = 3;
  static double constexpr kMinAccurateGpsHeadingSpeedMps = 1;
  static double constexpr kMaxGpsHeadingErrorDeg = 10;
  // A longer pause of the source is a lost connection, the car could do anything meanwhile.
  static double constexpr kMaxMotionDtSec = 1;
  // A second the car stands in goes to the trip log if the car has rotated at least this much in it.
  static double constexpr kGyroLogMinRotationDeg = 1;
  static int64_t constexpr kSnapIntervalMs = 1000;
  // A road farther aside is another street: the car doesn't jump to it, however inaccurate the position is.
  // It did at the end of a street with the position ahead of the car, and between the ramps of a junction.
  // Off the roads the car is stopped (kLostRoadM) until the user marks it.
  static double constexpr kSnapRadiusM = 20;
  // The road the car is put on when the user marks it or the movement resumes may be farther.
  static double constexpr kMaxSnapRadiusM = 60;
  // The bends of the road are compared with the rotation of the car this often: it takes tens of road lookups.
  static int64_t constexpr kBendMatchIntervalMs = 2000;
  // The position is searched this far along the road at least, and never farther than the maximum: a longer
  // road may have a similar bend.
  static double constexpr kMinBendSearchM = 30;
  static double constexpr kMaxBendShiftM = 60;
  // Smaller errors are within the accuracy of the bends on the map.
  static double constexpr kMinBendShiftM = 8;
  // A slow car weaves around holes and parked cars, that looks like a bend of the road.
  static double constexpr kMinBendMoveSpeedMps = 25 / 3.6;
  // What the accelerometer has added to the position fades away in this time.
  static double constexpr kAccelFadeSec = 10;
  // A turn at a crossing rotates the car faster than a bend of a road.
  static double constexpr kTurnStartRateDeg = 10;
  // The turn is over when the car goes straight for a while.
  static double constexpr kTurnEndRateDeg = 4;
  static int64_t constexpr kTurnEndCalmNs = 1'000'000'000;
  // The car starts turning a few meters before the middle of the crossing.
  static double constexpr kTurnCornerAheadM = 5;
  // The car is moved to the new road found this close to the moved position.
  static double constexpr kTurnSnapRadiusM = 20;
  // A car that was on a road before the turn and is on a road going the new way after it has followed the
  // roads through the turn. A crossing moving it farther than this is another one: the right one is not a
  // crossing for the map, e.g. a road branching off at a small angle. On the trips checked such a move brought
  // the car closer to GPS in 2 of 18 and farther from it in 13.
  static double constexpr kMaxFollowedTurnMoveM = 5;
  // A car turning at a crossing rotates one way. Rotating much more than it has turned, it has driven around
  // a roundabout or along a winding road, and the turn is not at the crossing where it has started.
  static double constexpr kMaxExtraTurnRotationDeg = 60;
  // The pause is over when the car drives this fast for a while: nobody drives so fast backwards.
  static int constexpr kAutoResumeSpeedKmh = 15;
  static int64_t constexpr kAutoResumeMs = 3000;
  // A car drives this far off the roads only in a yard or a parking lot missing on the map, or the calculation
  // has gone wrong, e.g. with a wrong heading. The car is stopped on the road then until the user marks it.
  static double constexpr kLostRoadM = 30;
  // The user is reminded to mark the car while it drives with the position stopped, e.g. on the next trip.
  static int64_t constexpr kRoadLostReminderMs = 30'000;
  // A car driving this slowly off the roads maneuvers: it parks, or drives through a yard or a parking lot missing
  // on the map. The calculation gone wrong takes a car off the roads at any speed.
  static double constexpr kMaxManeuverSpeedMps = 20 / 3.6;
  // A maneuvering car this far off the roads has left them: the movement is paused by itself, the mark waits on
  // the road and the car is followed apart. On the trips checked a car that was not parking drove so far off the
  // roads that slowly twice an hour, for a few seconds.
  static double constexpr kManeuverPauseM = 15;
  // A maneuvering car is not thrown to a road this far aside at once, as a car driving along a road is: next to a
  // yard that road is a street behind the houses. 13 of 31 maneuvers at a parking place had such a throw.
  static double constexpr kMaxManeuverSnapM = 10;
  // It is put on the road it has driven along for this distance without turning.
  static double constexpr kRoadConfirmM = 15;
  static double constexpr kMaxRoadConfirmTurnDeg = 15;
  // A road going this much another way is another road.
  static double constexpr kMaxRoadConfirmBearingDiffDeg = 10;
  // The car speed has no sign. Which way the car has started, the accelerometer of the box tells: the speed it
  // has felt since the car stood, by the time the car reports that it moves and a little later. The car reports
  // it late by the lag of its speed, so it stood for sure until the lag and a margin before that. On the trips
  // checked 43 of 50 starts were felt as 0.5 m/s forward or more, 4 at a parking place as 0.3 m/s backward or
  // more, 3 were in between.
  static double constexpr kReverseStartSpeedMps = 0.3;
  static int64_t constexpr kStartFeltAfterMs = 300;
  static int64_t constexpr kStandMarginMs = 300;
  // What the accelerometer feels while the car stands is taken from this time at most and at least.
  static int64_t constexpr kMaxStandFeltMs = 2000;
  static int64_t constexpr kMinStandFeltMs = 300;
  // The accelerometer is compared with the last seconds of the car speed, which has no sign: it is not learned
  // from and doesn't tell how much faster the car is while the car drives backwards and for this time after it.
  static int64_t constexpr kAccelAfterBackMs = 7000;
  // What is learned about the accelerometer of the box is saved this often.
  static int64_t constexpr kBoxAccelSaveMs = 60'000;
  // The speed table is saved once in a while, not on every position.
  static int constexpr kSpeedTableSaveSamples = 30;

  /// \param roads the roads and the followed route, the car is kept on them.
  InertialNavigator(Delegate & delegate, Scheduler & scheduler, Clock const & clock, Storage & storage, Roads & roads,
                    Listener & listener);
  ~InertialNavigator() override;

  /// Starts the motion source chosen in the settings.
  void Start();
  void Stop();

  /// Starts the gyroscope calibration. The gyroscope must be fixed and the car must stand still for ~3 s.
  void Calibrate();

  /// A trusted GPS position. The inertial navigation continues from it.
  void OnGpsPosition(Fix const & gps);
  /// The position is on a road: its direction is more reliable than the calculated one.
  /// \param bearingDeg direction of the road in the direction of the movement.
  void SetRoadPosition(ms::LatLon const & position, double bearingDeg);
  /// The user has corrected the lag of the calculated position along the road.
  /// \param appliedM the distance the position was moved by, negative if it was moved back.
  void OnPositionCorrected(ms::LatLon const & position, double bearingDeg, double appliedM);
  /// Trusted GPS measures the real speed of the car: the car speed errors at different speeds are learned
  /// from it. It works in the manual mode and when GPS is disabled for tests too.
  void OnSpeedTableGps(Fix const & gps);
  /// Forgets the car speed errors measured by GPS and corrected by the user, e.g. after new tyres.
  void ClearSpeedCalibration();
  /// Moves the position without changing the heading, e.g. to the shown position while the inertial
  /// navigation doesn't work yet.
  void SetPosition(ms::LatLon const & position);
  /// \param headingDeg the direction the car looks at, clockwise from the north.
  void SetHeading(double headingDeg, HeadingSource source);
  /// Pauses the movement of the car while it maneuvers, or resumes it: the car is put on the road where it
  /// stands, looking the way it has turned to during the pause.
  void SetPaused(bool paused);
  /// \returns true if the navigation has paused the movement by itself: the car has left the roads maneuvering.
  /// Its mark waits on the road until the car drives along a road again, or the user resumes the movement.
  bool IsPausedByItself() const { return m_pausedByItself; }
  /// \returns where the car is by the calculation while the pause set by itself keeps its mark on the road.
  std::optional<ms::LatLon> const & GetOffRoadCar() const { return m_offRoadCar; }
  /// \returns true if the car drives backwards by the accelerometer of the box.
  bool IsDrivingBack() const { return m_direction < 0; }

  // The platform events, passed to the source they are for.
  void OnGyro(int64_t timestampNs, Vec3 const & gyro);
  void OnAccel(int64_t timestampNs, Vec3 const & accel);
  void OnElm327Connected();
  void OnElm327Bytes(std::string_view data);
  void OnElm327Closed(std::string const & reason);
  void OnEsp32BleBytes(std::string_view bytes);
  void OnEsp32BleState(BleState state);

  bool IsReady() const;
  bool HasPosition() const { return m_deadReckoning.HasPosition(); }
  std::optional<Fix> GetLocation() const;
  /// \returns the car direction, clockwise from the north.
  std::optional<double> GetHeading() const;
  HeadingSource GetHeadingSource() const { return m_headingSource; }
  /// \returns the ratio the current speed from the car is multiplied by, 1 if it is not corrected yet.
  double GetSpeedScale() const;
  /// \returns how late the car reports its speed, seconds: measured by GPS, or the usual one until then.
  double GetSpeedLag() const { return m_speedLag.Get(); }
  bool IsSpeedLagMeasured() const { return m_speedLag.IsMeasured(); }
  /// \returns true if the accelerometer of the box tells the speed changes before the car does.
  bool IsBoxAccelUsed() const { return m_boxAccelSpeed.IsUsable(); }
  /// \returns how well the accelerometer of the box follows the car, 1 is the best.
  double GetBoxAccelCorrelation() const { return m_boxAccelSpeed.GetCorrelation(); }
  /// \returns how far the position is ahead of where the car speed alone would put it, meters.
  double GetAccelAheadM() const { return m_accelAheadM; }
  /// The same of the phone accelerometer, which is only watched: nothing if it doesn't follow the car now.
  double GetPhoneAccelCorrelation() const { return m_phoneAccelSpeed.GetCorrelation(); }
  std::optional<double> GetPhoneAccelAheadM() const;
  /// \returns how many speed ranges of the table are measured by GPS.
  int GetSpeedTableRanges() const { return m_speedTable.GetKnownRanges(); }
  std::string GetSpeedTableDescription() const { return m_speedTable.ToString(); }
  std::optional<CarInfo> GetCarInfo() const;
  SourceState GetSourceState() const;
  /// \returns the name of the sensor device to show, empty for the phone.
  std::string GetDeviceName() const;
  /// What the platform does about Bluetooth to reach the ESP32 sensor box.
  BleState GetBleState() const;
  /// \returns nothing if the car movement doesn't come from the ESP32 sensor box.
  std::optional<Esp32Source::DataRate> GetEsp32DataRate() const;
  /// \returns the ESP32 sensor box, nullptr if the source is another one.
  Esp32Source * GetEsp32() { return m_esp32; }
  /// \returns the last speed in km/h, or -1 if it is unknown or stale.
  int GetSpeedKmh() const;
  CalibrationState GetCalibrationState();
  int GetCalibrationProgressPercent() const;
  /// \returns how far the last turn has moved the car to its crossing.
  std::optional<double> GetLastTurnShiftM() const { return m_lastTurnShiftM; }
  double GetDistanceSinceFix() const { return m_deadReckoning.GetDistanceSinceFix(); }
  bool IsPaused() const { return m_paused; }
  /// \returns true if the car has left the roads and stands where it was on a road the last time.
  bool IsRoadLost() const { return m_roadLost; }
  /// \returns true if the last position was snapped to a road or the route.
  bool IsOnRoad() const { return m_onRoad; }
  /// \returns how far the closest road was from the calculated position.
  std::optional<double> GetLastSnapShiftM() const { return m_lastSnapShiftM; }
  /// \returns how much to the right of that road the car looked, degrees.
  std::optional<double> GetLastSnapBearingDiffDeg() const { return m_lastSnapBearingDiffDeg; }
  SnapResult GetLastSnapResult() const { return m_lastSnapResult; }

  // For the trip log.
  /// \returns true while the car makes a turn that is to be moved to its crossing.
  bool IsTurning() const { return m_turning; }
  /// \returns all the car has driven, meters.
  double GetPathM() const { return m_pathM; }
  /// \returns how far and how fast at most the car has driven off the roads since it was on a road.
  double GetOffRoadM() const { return m_offRoadM; }
  double GetOffRoadMaxSpeedMps() const { return m_offRoadMaxSpeedMps; }
  /// \returns how far the maneuvering car has driven along the road it is to be put on, nothing without one.
  std::optional<double> GetRoadCandidateM() const;

private:
  // MotionSource::Listener overrides:
  void OnSpeed(int speedKmh, int64_t timeMs) override;
  void OnMotion(double yawDeltaDeg, double dtSec, int64_t timestampNs) override;
  void OnSourceAccel(std::array<double, 3> const & accel) override;

  void CheckTurnSign(Fix const & gps);
  double GetSpeedScale(int speedKmh) const;
  void SaveSpeedTable();
  bool IsSpeedFresh() const;
  void SetRoadPoint(ms::LatLon const & position);
  void TrackOffRoad(double distanceM);
  void NotifyRoadLost();
  void PauseByItself();
  void EndPauseByItself();
  // Tells by the accelerometer of the box which way the car drives, forward or backward.
  void TrackDirection();
  void SetDirection(int direction);
  // Nothing is known about the way the car drives: it drives forward.
  void ResetDirection();
  bool HasDrivenBackLately() const;
  void ResetLag();
  void RestoreBoxAccel();
  void SaveBoxAccel();
  // Another box is fixed in another way: what is learned about the accelerometer is of the box it was learned with.
  void CheckBoxAccel();
  // The car paused by itself is put on the road it drives along, if there is one.
  void CheckBackOnRoad();
  /// \returns true if the maneuvering car has driven along the road far enough to be put on it.
  bool IsRoadConfirmed(RoadPoint const & road, double headingDeg);
  // The car is on the road: the maneuver is over.
  void PutOnRoad(ms::LatLon const & point, double bearingDeg);
  double DistanceWithLag(double dtSec);
  // Rotates the car, following its turn at a crossing if |trackTurn|.
  void Rotate(double yawDeltaDeg, double dtSec, int64_t timestampNs, bool trackTurn);
  void SnapToRoad();
  void MatchBend();
  void TrackTurn(double yawDeltaDeg, double dtSec, int64_t timestampNs);
  void MatchTurn(int64_t timestampNs);

  Delegate & m_delegate;
  Scheduler & m_scheduler;
  Clock const & m_clock;
  Storage & m_storage;
  Roads & m_roads;
  Listener & m_listener;

  std::unique_ptr<MotionSource> m_source;
  // The same source as m_source if it is of this type, to pass the platform events to it.
  PhoneMotion * m_phone = nullptr;
  Esp32Source * m_esp32 = nullptr;
  bool m_started = false;

  int64_t m_lastSnapMs = 0;
  bool m_onRoad = false;
  std::optional<double> m_lastSnapShiftM;
  std::optional<double> m_lastSnapBearingDiffDeg;
  SnapResult m_lastSnapResult = SnapResult::None;
  // Where the car was on a road or at a known position the last time, and the distance driven off the roads
  // since then.
  std::optional<ms::LatLon> m_roadPoint;
  double m_offRoadM = 0;
  // The car has left the roads and stands at the last road point until it is marked or GPS comes back.
  bool m_roadLost = false;
  int64_t m_roadLostNotifiedMs = 0;

  DeadReckoning m_deadReckoning;
  HeadingSource m_headingSource = HeadingSource::None;
  int64_t m_lastOutputMs = 0;
  int m_speedKmh = -1;
  int64_t m_speedTimeMs = 0;

  // The speed error estimated from the corrections of the user, used until GPS measures it.
  SpeedScale m_speedScale;
  TurnSignChecker m_turnSignChecker;
  BendMatcher m_bendMatcher;
  int64_t m_lastBendMatchMs = 0;
  // The speed errors measured by trusted GPS at different speeds.
  SpeedTable m_speedTable;
  SpeedLag m_speedLag;
  // The accelerometer for the trip log, nothing depends on it yet.
  AccelLog m_accelLog;
  // The rotation of the car for the trip log.
  GyroLog m_gyroLog;
  bool m_moving = false;
  // A car reporting that it stands doesn't turn. But it reports that it moves late by the lag of its speed, and
  // a car starting at a crossing turns at once: its rotation of that time and a margin is kept, the time and the
  // degrees, and is its turn when it reports that it moves. On a turn started from a standstill 15 degrees of
  // 68 were lost that way.
  std::deque<std::pair<int64_t, double>> m_standRotation;
  // The speed the lag is compensated up to, nothing if the car didn't move the position.
  std::optional<double> m_lagSpeedMps;
  // The position is to be moved back by this distance yet: it is not moved back, it waits.
  double m_lagPendingM = 0;
  // An accelerometer feels the car speeding up and braking before it reports the speed. The one of the box
  // is fixed in the car and is used. The phone one is only watched and written to the trip log: on the trips
  // checked it added centimeters, and a phone taken from its holder would move the position by meters.
  AccelSpeed m_boxAccelSpeed;
  AccelSpeed m_phoneAccelSpeed;
  // How far the accelerometer has put the position ahead of where the changes of the car speed would. It
  // fades away: both tell the same distance in the end, so an error of the accelerometer doesn't stay.
  double m_accelAheadM = 0;
  // The same the phone accelerometer would do.
  double m_phoneAheadM = 0;
  // The last GPS position the speed errors were measured by.
  std::optional<int64_t> m_speedTableGpsMs;
  double m_speedTableGpsSpeedMps = 0;
  int m_unsavedSpeedSamples = 0;

  // The turn the car is making now: where it has started and where the car looked before it.
  bool m_turning = false;
  double m_turnFromBearing = 0;
  ms::LatLon m_turnStart;
  // The car was on a road when the turn has started.
  bool m_turnFromRoad = false;
  // The turn has been calm since then: the car goes straight.
  std::optional<int64_t> m_turnCalmSinceNs;
  // The rotation of the turn both ways together.
  double m_turnRotationDeg = 0;
  // For the trip log: when the turn has started, all the car had driven by then, and its speeds in the turn.
  int64_t m_turnStartNs = 0;
  double m_turnStartPathM = 0;
  int m_turnMinKmh = 0;
  int m_turnMaxKmh = 0;
  // How far the last turn has moved the car to its crossing, for the log of a drive.
  std::optional<double> m_lastTurnShiftM;

  // While the car maneuvers, e.g. parks or turns around in several moves, the speed from the car is always
  // positive and would move the car forward. The car stays where it is then, the gyroscope still follows
  // its rotation.
  bool m_paused = false;
  // The pause is set by the navigation itself, not by the user: the car has left the roads maneuvering.
  bool m_pausedByItself = false;
  // Where the car is by the calculation while the pause set by itself keeps its mark on the road.
  std::optional<ms::LatLon> m_offRoadCar;
  // The fastest the car has driven off the roads since the last road point.
  double m_offRoadMaxSpeedMps = 0;
  // The road the maneuvering car seems to drive along: its direction, and the path of the car and where the car
  // looked when the road was found.
  struct RoadCandidate
  {
    double m_bearingDeg;
    double m_pathM;
    double m_headingDeg;
  };
  std::optional<RoadCandidate> m_roadCandidate;
  // All the car has driven, paused or not.
  double m_pathM = 0;
  // The way the car drives since its last start: 1 forward, -1 backward. A standing car is ready to drive forward.
  int m_direction = 1;
  // Since when it reports that it stands, and when it has reported that it moves after that.
  std::optional<int64_t> m_standMs;
  std::optional<int64_t> m_startMs;
  // The way of the start is told by the accelerometer already.
  bool m_startFelt = true;
  // Driven since the start, either way, and how far the mark of the car was moved for that.
  double m_startM = 0;
  double m_startMarkM = 0;
  // When it drove backwards the last time.
  std::optional<int64_t> m_lastBackMs;
  // The box the accelerometer is learned with, and when it was saved.
  std::string m_boxAccelId;
  int64_t m_boxAccelSavedMs = 0;
  // Since when the car drives fast enough to end the pause.
  std::optional<int64_t> m_fastSinceMs;
};

std::string DebugPrint(InertialNavigator::HeadingSource source);
std::string DebugPrint(InertialNavigator::SnapResult result);
}  // namespace nogps

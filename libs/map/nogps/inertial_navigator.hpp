#pragma once

#include "map/nogps/accel_log.hpp"
#include "map/nogps/accel_speed.hpp"
#include "map/nogps/bend_matcher.hpp"
#include "map/nogps/dead_reckoning.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/esp32_source.hpp"
#include "map/nogps/gyro_calibrator.hpp"
#include "map/nogps/motion_source.hpp"
#include "map/nogps/path_matcher.hpp"
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
  // Roads going apart by this much around the car are a fork: a ramp, a slip road, an exit of a roundabout. Less
  // is the same road or two carriageways, more is a crossing, which a turn of the car tells.
  static double constexpr kMinForkDeg = 3;
  static double constexpr kMaxForkDeg = 35;
  // At a fork the branch is told by the heading: the branches begin at one point, so the distance to them tells
  // nothing, and the route the car follows takes one of them whatever the car does. On October 11 it pulled the
  // mark to its branch 50 m away from the car. A degree between a branch and the heading is worth this many
  // meters to the branch, 5 times more than elsewhere.
  static double constexpr kForkMetersPerDeg = 1;
  // Places of roads closer than this are the same branch.
  static double constexpr kSameForkBranchM = 2;
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
  // Unless the bends tell the same place several times while the car drives on. On October 14 they told the mark was
  // 12-14 m ahead six times in 20 s at 16-26 km/h, and nothing else told the place for 6 km. The trips of October
  // 11-14 driven again with this rule moved a slow car 4 times: the bends told it was in place after every move, the
  // one with GPS brought the mark from 8 m to 2 m from it.
  static int constexpr kSlowBendCount = 3;
  static double constexpr kSlowBendAgreeM = 3;
  static double constexpr kSlowBendDriveM = 20;
  // What the accelerometer has added to the position fades away in this time.
  static double constexpr kAccelFadeSec = 10;
  // A turn at a crossing rotates the car faster than a bend of a road.
  static double constexpr kTurnStartRateDeg = 10;
  // The user marks the car on the road it turns into before the turn is over: the mark looks along the road at
  // once, and the rest of the turn has turned it away from that road. On October 11 a mark set 81 degrees before
  // the end of a turn was turned by the other 79 and looked across the road; the user had to mark it again. A car
  // marked this far or farther from the direction of the road while it turns is finishing that turn, if the road
  // is farther the way it was turning: a mark the other way, as another one of that day, means the heading was
  // wrong. That car crept on for a second, stood for three and turned on: the turn is over when the car has
  // driven this far without turning, not after a while.
  static double constexpr kMinMarkTurnDeg = 10;
  static double constexpr kMarkTurnEndM = 15;
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
  // A car driving around a roundabout is off the road of the map for a while: on a small circle a few meters of
  // the calculated distance are tens of degrees between the car and the road, and the car goes straight for a
  // moment in the middle of it, which ends its turn. With a roundabout this close, and for some distance after
  // it, the car is neither lost nor maneuvering. On October 12 a car was stopped as lost 4 s before it left a
  // roundabout the way it had driven in.
  static double constexpr kRoundaboutRadiusM = 30;
  // The roads leaving a roundabout bend for tens of meters, and the car that came to it with the distance wrong
  // by 20-30 m leaves it that far from its road. That trip driven again, the roundabout was 30 m behind and the
  // car was lost 5 s before it went straight. It leaves a roundabout for this distance, and is put on the road
  // going its way as far aside as a mark of the user is, after it has driven along it without turning.
  static double constexpr kRoundaboutExitM = 100;
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
  // A car maneuvering at a parking place stands for a second between driving back and forth, too short to feel
  // it standing: what was felt at its last longer stand is taken, while the car is about there and the slope
  // under it is the same. On the trips checked every such start was within 20 s and 16 m of that stand.
  static int64_t constexpr kKeepStandFeltMs = 30'000;
  static double constexpr kKeepStandFeltM = 20;
  // The first jerk of a start tells the wrong way sometimes, e.g. of a car rolling back before it starts uphill.
  // For this long after the start the speed the accelerometer has felt is compared with the one the car reports,
  // which is the speed of the lag ago. Felt clearly the other way for a while, the car drives the other way. Felt
  // much faster than the car reports, it is the slope that has changed under the car: on a ramp going down a car
  // driving forward is felt as driving backwards 3 s after its start, so only the first seconds tell that. On
  // the trips of October 11-12 GPS has seen the way of 43 starts: 33 were told right before, 41 with this.
  static int64_t constexpr kStartCheckMs = 4000;
  static int64_t constexpr kStartCheckBackMs = 2000;
  static int64_t constexpr kStartCheckHoldMs = 500;
  static double constexpr kMinFeltSpeedRatio = 0.5;
  static double constexpr kMaxFeltSpeedRatio = 2;
  static double constexpr kMaxFeltSpeedExtraMps = 0.5;
  // The accelerometer is compared with the last seconds of the car speed, which has no sign: it is not learned
  // from and doesn't tell how much faster the car is while the car drives backwards and for this time after it.
  // A start backward is not always told, so it is learned from at a speed nobody drives backwards at only: a
  // parking maneuver has made it useless for the next 6 minutes of a trip.
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
  /// The user has marked the car on a road, the mark looks along it. A car turning at that moment is turning
  /// into the marked road: the rest of its turn is not a turn away from that road.
  void SetMarkOnRoad(ms::LatLon const & position, double bearingDeg);
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
  /// \returns false if the pause set by itself stays: the car has maneuvered away from its mark, and there is no
  /// road next to it to go on from.
  bool SetPaused(bool paused);
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
  /// \returns how long ago the car has measured that speed, -1 if it never did.
  int64_t GetSpeedAgeMs() const;
  CalibrationState GetCalibrationState();
  int GetCalibrationProgressPercent() const;
  /// \returns how far the last turn has moved the car to its crossing.
  std::optional<double> GetLastTurnShiftM() const { return m_lastTurnShiftM; }
  double GetDistanceSinceFix() const { return m_deadReckoning.GetDistanceSinceFix(); }
  bool IsPaused() const { return m_paused; }
  /// \returns true if the car has left the roads and stands where it was on a road the last time.
  bool IsRoadLost() const { return m_roadLost; }
  /// \returns true if the mark waits for the user to put it on a road: the road is lost, or the car drives fast
  /// during the pause set by itself and no road goes its way around.
  bool IsUserNeeded() const { return m_roadLost || (m_pausedByItself && m_fastWithoutRoad); }
  /// \returns true if the last position was snapped to a road or the route.
  bool IsOnRoad() const { return m_onRoad; }
  /// \returns how far the closest road was from the calculated position.
  std::optional<double> GetLastSnapShiftM() const { return m_lastSnapShiftM; }
  /// \returns how much to the right of that road the car looked, degrees.
  std::optional<double> GetLastSnapBearingDiffDeg() const { return m_lastSnapBearingDiffDeg; }
  SnapResult GetLastSnapResult() const { return m_lastSnapResult; }

  // For the trip log.
  /// \returns true while the car is off the roads next to a roundabout, where it is not lost.
  bool IsAtRoundabout() const { return m_atRoundabout; }
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
  void LogSpeedCalibration() const;
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
  // Looks if the car that is not on a road is next to a roundabout.
  void CheckRoundabout();
  void LeaveRoundabout();
  // The car paused by itself is put on the road it drives along, if there is one.
  void CheckBackOnRoad();
  /// \returns true if the car has driven along the road far enough to be put on it. |candidate| is that road
  /// while the car drives along it.
  struct RoadCandidate;
  bool IsRoadConfirmed(RoadPoint const & road, double headingDeg, std::optional<RoadCandidate> & candidate);
  /// \returns true if the maneuvering car has driven along the road far enough to be put on it.
  bool IsRoadConfirmed(RoadPoint const & road, double headingDeg);
  // The car is on the road: the maneuver is over.
  void PutOnRoad(ms::LatLon const & point, double bearingDeg);
  double DistanceWithLag(double dtSec);
  // Rotates the car, following its turn at a crossing if |trackTurn|.
  // \returns what the car is rotated by, see AbsorbMarkTurn().
  double Rotate(double yawDeltaDeg, double dtSec, int64_t timestampNs, bool trackTurn);
  // \returns the part of the rotation that is not the rest of the turn the car was marked in.
  double AbsorbMarkTurn(double yawDeltaDeg, double dtSec);
  void SnapToRoad();
  // \returns the branch the car drives along by its heading if the road forks at the position.
  std::optional<RoadPoint> ChooseBranch(ms::LatLon const & position, double headingDeg);
  void MatchBend();
  // \returns true if the car has been moved to where the way of a turn around lies on the roads, see PathMatcher.
  bool MatchLoop(double toBearingDeg);
  // \returns true if the bends of a slow car have told the same place enough times, see kSlowBendCount.
  bool IsSlowBendConfirmed(double aheadM);
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
  // The car is off the roads next to a roundabout or leaving it, see kRoundaboutRadiusM. What it had driven
  // when the roundabout was next to it the last time, and the road it leaves the roundabout by, if it is found.
  bool m_atRoundabout = false;
  std::optional<double> m_roundaboutPathM;
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
  PathMatcher m_pathMatcher;
  int64_t m_lastBendMatchMs = 0;
  // The places told by the bends of a slow car since the position was set.
  struct SlowBends
  {
    int m_fixCount = 0;
    double m_firstAheadM = 0;
    double m_firstDistanceM = 0;
    int m_count = 0;
  };
  std::optional<SlowBends> m_slowBends;
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
  // What is left of the turn the car was marked in, see kMinMarkTurnDeg, and what the car had driven when it
  // began to go straight.
  std::optional<double> m_markTurnLeftDeg;
  std::optional<double> m_markTurnCalmPathM;
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
  // The car has driven fast during that pause where no road goes its way.
  bool m_fastWithoutRoad = false;
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
  // The road the car leaves a roundabout by, see kRoundaboutExitM.
  std::optional<RoadCandidate> m_exitCandidate;
  // All the car has driven, paused or not.
  double m_pathM = 0;
  // The way the car drives since its last start: 1 forward, -1 backward. A standing car is ready to drive forward.
  int m_direction = 1;
  // Since when it reports that it stands, and when it has reported that it moves after that.
  std::optional<int64_t> m_standMs;
  std::optional<int64_t> m_startMs;
  // The way of the start is told by the accelerometer already.
  bool m_startFelt = true;
  // Until when the car stood for sure before that start.
  int64_t m_startStandToMs = 0;
  // What the accelerometer of the box has felt along the car standing, m/s2: at the last stand long enough to
  // feel it, when that was and what the car had driven by then.
  std::optional<double> m_standAccel;
  int64_t m_standAccelMs = 0;
  double m_standAccelPathM = 0;
  // Since when the accelerometer feels the car driving the other way than it is known to drive.
  std::optional<int64_t> m_otherWayMs;
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

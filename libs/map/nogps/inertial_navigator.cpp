#include "map/nogps/inertial_navigator.hpp"

#include "map/nogps/clock.hpp"
#include "map/nogps/esp32_source.hpp"
#include "map/nogps/geo.hpp"
#include "map/nogps/phone_motion.hpp"
#include "map/nogps/road_walker.hpp"
#include "map/nogps/roads.hpp"
#include "map/nogps/storage.hpp"
#include "map/nogps/turn_matcher.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nogps
{
InertialNavigator::InertialNavigator(Delegate & delegate, Scheduler & scheduler, Clock const & clock, Storage & storage,
                                     Roads & roads, Listener & listener)
  : m_delegate(delegate)
  , m_scheduler(scheduler)
  , m_clock(clock)
  , m_storage(storage)
  , m_roads(roads)
  , m_listener(listener)
{
  m_speedTable.Deserialize(m_storage.Get<std::string>(Storage::kSpeedTable, ""));
  m_speedLag.Deserialize(m_storage.Get<std::string>(Storage::kSpeedLag, ""));
}

InertialNavigator::~InertialNavigator()
{
  Stop();
}

void InertialNavigator::Start()
{
  if (m_started)
    Stop();
  m_started = true;
  // The speedometer error is the same on every trip.
  m_speedScale.Set(m_storage.Get<double>(Storage::kSpeedScale, 1));
  LogSpeedCalibration();
  // The source may have changed: a sensor box with another gyroscope or the phone.
  m_turnSignChecker = {};
  m_bendMatcher = {};
  bool const esp32 = m_storage.IsEsp32Source();
  MotionSource::Listener & listener = *this;
  if (esp32)
  {
    auto source = std::make_unique<Esp32Source>(m_delegate, m_scheduler, m_clock, listener);
    m_esp32 = source.get();
    m_source = std::move(source);
  }
  else
  {
    std::optional<std::string> address;
    if (auto const saved = m_storage.Get<std::string>(Storage::kElm327Address, ""); !saved.empty())
      address = saved;
    auto source = std::make_unique<PhoneMotion>(m_delegate, m_scheduler, m_clock, address, listener);
    m_phone = source.get();
    m_source = std::move(source);
  }
  m_source->Start();
  // The accelerometer goes to the trip log with any source.
  m_accelLog.Reset();
  m_gyroLog.Reset();
  // The phone may be put into the holder another way. The box stays in its place: what is learned about it goes on.
  m_boxAccelSpeed.Reset();
  m_boxAccelId.clear();
  if (esp32)
    RestoreBoxAccel();
  m_phoneAccelSpeed.Reset();
  ResetLag();
  ResetDirection();
  m_lastBackMs.reset();
  m_standRotation.clear();
  m_delegate.StartMotionSensors(!esp32 /* gyroscope */);
}

void InertialNavigator::Stop()
{
  if (!m_started)
    return;
  m_started = false;
  if (m_esp32)
    SaveBoxAccel();
  m_delegate.StopMotionSensors();
  m_source->Stop();
  m_phone = nullptr;
  m_esp32 = nullptr;
  m_source.reset();
  m_speedKmh = -1;
  SaveSpeedTable();
}

void InertialNavigator::Calibrate()
{
  LOG(LINFO, ("Calibrate"));
  if (m_source)
    m_source->Calibrate();
}

void InertialNavigator::OnGpsPosition(Fix const & gps)
{
  EndPauseByItself();
  m_deadReckoning.SetPosition(gps.m_position);
  SetRoadPoint(gps.m_position);
  m_turning = false;
  // GPS bearing is noisy at low speeds, unless GPS tells it is accurate.
  bool const goodBearing = gps.m_bearingDeg && gps.m_speedMps &&
                           (*gps.m_speedMps >= kMinGpsHeadingSpeedMps ||
                            (*gps.m_speedMps >= kMinAccurateGpsHeadingSpeedMps && gps.m_bearingAccuracyDeg &&
                             *gps.m_bearingAccuracyDeg <= kMaxGpsHeadingErrorDeg));
  if (goodBearing)
  {
    CheckTurnSign(gps);
    SetHeading(*gps.m_bearingDeg, HeadingSource::Gps);
  }
  else
  {
    m_turnSignChecker.OnNoGpsBearing();
  }
}

void InertialNavigator::CheckTurnSign(Fix const & gps)
{
  auto const result = m_turnSignChecker.OnGpsBearing(*gps.m_bearingDeg, gps.m_timeMs);
  if (!result)
    return;
  if (*result == TurnSignChecker::Result::Reversed)
  {
    LOG(LERROR, ("The gyroscope turns the car the other way than GPS, same turns =", m_turnSignChecker.GetSameTurns(),
                 "opposite turns =", m_turnSignChecker.GetOppositeTurns()));
    m_listener.OnTurnsReversed();
  }
  else
  {
    LOG(LINFO, ("The gyroscope turns the car as GPS does, same turns =", m_turnSignChecker.GetSameTurns(),
                "opposite turns =", m_turnSignChecker.GetOppositeTurns()));
  }
}

void InertialNavigator::SetRoadPosition(ms::LatLon const & position, double bearingDeg)
{
  // The car is where it is told to be, not where it has maneuvered to by the calculation.
  EndPauseByItself();
  m_deadReckoning.SetPosition(position);
  SetRoadPoint(position);
  m_turning = false;
  SetHeading(bearingDeg, HeadingSource::Road);
}

void InertialNavigator::OnPositionCorrected(ms::LatLon const & position, double bearingDeg, double appliedM)
{
  m_speedScale.OnCorrection(appliedM);
  m_storage.Set(Storage::kSpeedScale, m_speedScale.Get());
  LOG(LINFO, ("Corrected by", std::round(appliedM), "m, speed scale =", m_speedScale.Get()));
  SetRoadPosition(position, bearingDeg);
}

double InertialNavigator::GetSpeedScale() const
{
  return GetSpeedScale(std::max(0, m_speedKmh));
}

double InertialNavigator::GetSpeedScale(int speedKmh) const
{
  return m_speedTable.GetScale(speedKmh).value_or(m_speedScale.Get());
}

void InertialNavigator::OnSpeedTableGps(Fix const & gps)
{
  if (!gps.m_speedMps || !IsSpeedFresh())
  {
    m_speedTableGpsMs.reset();
    return;
  }
  // Several providers give the same position.
  if (m_speedTableGpsMs && gps.m_timeMs <= *m_speedTableGpsMs)
    return;
  m_speedLag.OnGpsSpeed(gps.m_timeMs, *gps.m_speedMps);
  if (m_speedTableGpsMs)
  {
    double const dt = (gps.m_timeMs - *m_speedTableGpsMs) / 1000.0;
    m_speedTable.OnSample(m_speedKmh, *gps.m_speedMps, dt, (*gps.m_speedMps - m_speedTableGpsSpeedMps) / dt);
    if (++m_unsavedSpeedSamples >= kSpeedTableSaveSamples)
      SaveSpeedTable();
  }
  m_speedTableGpsMs = gps.m_timeMs;
  m_speedTableGpsSpeedMps = *gps.m_speedMps;
}

void InertialNavigator::SaveSpeedTable()
{
  m_unsavedSpeedSamples = 0;
  m_storage.Set(Storage::kSpeedTable, m_speedTable.Serialize());
  m_storage.Set(Storage::kSpeedLag, m_speedLag.Serialize());
  LogSpeedCalibration();
}

void InertialNavigator::LogSpeedCalibration() const
{
  // For the trip log: the path of a trip is calculated again with these.
  LOG(LINFO, ("Speed table =", m_speedTable.ToString(), "lag =", m_speedLag.Get(), "measured =",
              m_speedLag.IsMeasured(), "scale =", m_speedScale.Get()));
}

void InertialNavigator::ClearSpeedCalibration()
{
  LOG(LINFO, ("Speed table was", m_speedTable.ToString()));
  m_speedTable.Clear();
  m_speedLag.Clear();
  SaveSpeedTable();
  m_speedScale.Reset();
  m_storage.Set(Storage::kSpeedScale, 1.0);
}

void InertialNavigator::SetPosition(ms::LatLon const & position)
{
  EndPauseByItself();
  m_deadReckoning.SetPosition(position);
  SetRoadPoint(position);
  if (m_deadReckoning.HasHeading())
    return;
  // Without GPS and marks the car looks along its road either way, the user turns it around if it is wrong.
  if (auto const road = m_roads.Snap(position, {}, kMaxSnapRadiusM))
    SetHeading(road->m_bearingDeg, HeadingSource::Road);
}

void InertialNavigator::SetHeading(double headingDeg, HeadingSource source)
{
  m_deadReckoning.SetHeading(headingDeg);
  m_headingSource = source;
}

std::optional<double> InertialNavigator::GetHeading() const
{
  if (!m_deadReckoning.HasHeading())
    return {};
  return m_deadReckoning.GetHeading();
}

bool InertialNavigator::IsReady() const
{
  return m_started && m_source->IsCalibrated() && IsSpeedFresh() && m_deadReckoning.IsReady();
}

std::optional<Fix> InertialNavigator::GetLocation() const
{
  if (!m_deadReckoning.HasPosition())
    return {};
  Fix fix;
  fix.m_provider = Provider::Inertial;
  fix.m_position = m_deadReckoning.GetPosition();
  fix.m_accuracyM = m_deadReckoning.GetAccuracy();
  fix.m_bearingDeg = GetHeading();
  fix.m_speedMps = m_deadReckoning.GetSpeed();
  fix.m_timeMs = m_clock.NowMs();
  fix.m_unixTimeMs = m_clock.UnixNowMs();
  return fix;
}

std::optional<CarInfo> InertialNavigator::GetCarInfo() const
{
  return m_source ? m_source->GetCarInfo() : std::nullopt;
}

SourceState InertialNavigator::GetSourceState() const
{
  return m_source ? m_source->GetState() : SourceState::Disconnected;
}

std::string InertialNavigator::GetDeviceName() const
{
  return m_source ? m_source->GetDeviceName() : std::string();
}

int InertialNavigator::GetSpeedKmh() const
{
  return IsSpeedFresh() ? m_speedKmh : -1;
}

CalibrationState InertialNavigator::GetCalibrationState()
{
  return m_source ? m_source->GetCalibrationState() : CalibrationState::None;
}

int InertialNavigator::GetCalibrationProgressPercent() const
{
  return m_source ? m_source->GetCalibrationProgressPercent() : 0;
}

int64_t InertialNavigator::GetSpeedAgeMs() const
{
  return m_speedKmh >= 0 ? m_clock.NowMs() - m_speedTimeMs : -1;
}

bool InertialNavigator::IsSpeedFresh() const
{
  return m_speedKmh >= 0 && m_clock.NowMs() - m_speedTimeMs < kSpeedStaleMs;
}

void InertialNavigator::SetRoadPoint(ms::LatLon const & position)
{
  m_roadPoint = position;
  m_offRoadM = 0;
  m_offRoadMaxSpeedMps = 0;
  m_roadLost = false;
}

void InertialNavigator::TrackOffRoad(double distanceM)
{
  // A turn is not counted: the car is aside from both roads for a while, and the turn is matched to its crossing
  // afterwards.
  if (m_onRoad || m_turning || !m_roadPoint || m_roadLost)
    return;
  m_offRoadM += distanceM;
  m_offRoadMaxSpeedMps = std::max(m_offRoadMaxSpeedMps, m_deadReckoning.GetSpeed());
  // A car driving along a road aside is put on it in a moment, see SnapToRoad(): it is not paused meanwhile.
  if (m_offRoadMaxSpeedMps <= kMaxManeuverSpeedMps &&
      (m_offRoadM >= kLostRoadM || (m_offRoadM >= kManeuverPauseM && !m_roadCandidate)))
  {
    PauseByItself();
    return;
  }
  if (m_offRoadM < kLostRoadM)
    return;
  LOG(LINFO, ("The road is lost after", std::round(m_offRoadM), "m, stopping at", *m_roadPoint));
  m_roadLost = true;
  m_deadReckoning.MoveTo(*m_roadPoint);
  NotifyRoadLost();
}

void InertialNavigator::NotifyRoadLost()
{
  m_roadLostNotifiedMs = m_clock.NowMs();
  m_listener.OnRoadLost();
}

void InertialNavigator::RestoreBoxAccel()
{
  auto const saved = m_storage.Get<std::string>(Storage::kBoxAccel, "");
  auto const colon = saved.find(':');
  if (colon == std::string::npos || !m_boxAccelSpeed.Deserialize(saved.substr(colon + 1)))
    return;
  m_boxAccelId = saved.substr(0, colon);
  LOG(LINFO, ("The accelerometer of the box", m_boxAccelId, "is known: usable =", m_boxAccelSpeed.IsUsable(),
              "correlation =", m_boxAccelSpeed.GetCorrelation()));
}

void InertialNavigator::SaveBoxAccel()
{
  m_boxAccelSavedMs = m_clock.NowMs();
  if (m_boxAccelSpeed.GetSamples() > 0)
    m_storage.Set(Storage::kBoxAccel, m_boxAccelId + ":" + m_boxAccelSpeed.Serialize());
}

void InertialNavigator::CheckBoxAccel()
{
  if (!m_esp32)
    return;
  if (auto const & info = m_esp32->GetInfo(); info && info->m_id != m_boxAccelId)
  {
    if (!m_boxAccelId.empty())
    {
      LOG(LINFO, ("The accelerometer was learned with the box", m_boxAccelId, "this one is", info->m_id));
      m_boxAccelSpeed.Reset();
    }
    m_boxAccelId = info->m_id;
  }
  int64_t const now = m_clock.NowMs();
  if (now - m_boxAccelSavedMs < kBoxAccelSaveMs)
    return;
  // For the trip log: where the accelerometer looks in its axes and how well it follows the car.
  char direction[64] = "-";
  if (auto const d = m_boxAccelSpeed.GetDirection())
    std::snprintf(direction, sizeof(direction), "%.3f,%.3f,%.3f", (*d)[0], (*d)[1], (*d)[2]);
  char buf[160];
  std::snprintf(buf, sizeof(buf), "dir=%s n=%.0f corr=%.2f recent=%.2f/%.2f usable=%d", direction,
                m_boxAccelSpeed.GetSamples(), m_boxAccelSpeed.GetCorrelation(), m_boxAccelSpeed.GetRecentCorrelation(),
                m_boxAccelSpeed.GetRecentScale(), m_boxAccelSpeed.IsUsable() ? 1 : 0);
  LOG(LINFO, ("BOXACC", buf));
  if (m_boxAccelSpeed.IsUsable())
    SaveBoxAccel();
  else
    m_boxAccelSavedMs = now;
}

void InertialNavigator::TrackDirection()
{
  int64_t const now = m_clock.NowMs();
  if (m_speedKmh == 0)
  {
    if (!m_standMs)
      m_standMs = now;
    m_startMs.reset();
    // The next start is a start forward until the accelerometer tells the other.
    m_direction = 1;
    return;
  }
  int64_t const lagMs = static_cast<int64_t>(m_speedLag.Get() * 1000);
  if (!m_startMs)
  {
    m_startMs = now;
    m_startM = 0;
    m_startMarkM = 0;
    m_startFelt = false;
    m_otherWayMs.reset();
    // The car stood for sure from when it has reported that until the lag of its speed before it has reported
    // that it moves.
    m_startStandToMs = now - lagMs - kStandMarginMs;
  }
  int direction = m_direction;
  if (!m_startFelt && now - *m_startMs >= kStartFeltAfterMs)
  {
    m_startFelt = true;
    // A start the accelerometer tells nothing about is a start forward.
    direction = 1;
    std::optional<double> stand;
    if (m_standMs)
    {
      int64_t const standFrom = std::max(*m_standMs, m_startStandToMs - kMaxStandFeltMs);
      if (m_startStandToMs - standFrom >= kMinStandFeltMs)
        stand = m_boxAccelSpeed.GetAccelAlong(standFrom, m_startStandToMs);
    }
    double const stoodSec = m_standMs ? (*m_startMs - *m_standMs) / 1000.0 : -1;
    m_standMs.reset();
    bool const kept = !stand && m_standAccel;
    if (stand)
    {
      m_standAccel = stand;
      m_standAccelMs = now;
      m_standAccelPathM = m_pathM;
    }
    else if (now - m_standAccelMs > kKeepStandFeltMs || m_pathM - m_standAccelPathM > kKeepStandFeltM)
    {
      m_standAccel.reset();
    }
    auto const start = m_boxAccelSpeed.GetAccelAlong(m_startStandToMs, now);
    if (m_standAccel && start)
    {
      double const feltMps = (*start - *m_standAccel) * (now - m_startStandToMs) / 1000.0;
      direction = feltMps <= -kReverseStartSpeedMps ? -1 : 1;
      LOG(LINFO, ("The car has started", direction > 0 ? "forward" : "backward", "the accelerometer has felt", feltMps,
                  "m/s, standing", *m_standAccel, "starting", *start, "m/s2, it stood", stoodSec, "s",
                  kept ? "too short, the stand before it is taken" : ""));
    }
    else
    {
      LOG(LINFO, ("The car has started, the accelerometer tells nothing: its direction is known =",
                  m_boxAccelSpeed.IsDirectionKnown(), "it stood", stoodSec, "s"));
    }
  }
  else if (m_startFelt && m_standAccel && now - *m_startMs <= kStartCheckMs)
  {
    // The car reports the speed it had the lag ago: what the accelerometer had felt by then is the same speed
    // with its sign.
    int64_t const tillMs = now - lagMs;
    auto const start = m_boxAccelSpeed.GetAccelAlong(m_startStandToMs, tillMs);
    double const feltMps = start ? (*start - *m_standAccel) * (tillMs - m_startStandToMs) / 1000.0 : 0;
    double const speedMps = m_speedKmh / 3.6;
    int const felt = feltMps < 0 ? -1 : 1;
    bool const clear = std::fabs(feltMps) >= std::max(kReverseStartSpeedMps, kMinFeltSpeedRatio * speedMps) &&
                       std::fabs(feltMps) <= kMaxFeltSpeedRatio * speedMps + kMaxFeltSpeedExtraMps;
    if (!clear || felt == direction || (felt < 0 && now - *m_startMs > kStartCheckBackMs))
    {
      m_otherWayMs.reset();
    }
    else if (!m_otherWayMs)
    {
      m_otherWayMs = now;
    }
    else if (now - *m_otherWayMs >= kStartCheckHoldMs)
    {
      m_otherWayMs.reset();
      direction = felt;
      LOG(LINFO, ("The car drives", direction > 0 ? "forward" : "backward", "by the accelerometer, it has felt",
                  feltMps, "m/s and the car reports", speedMps, "m/s,", now - *m_startMs, "ms after the start and",
                  std::round(m_startM), "m"));
    }
  }
  // Nobody drives that fast backwards.
  if (m_speedKmh >= kAutoResumeSpeedKmh && direction < 0)
  {
    LOG(LINFO, ("The car drives fast: it has driven forward since the start,", std::round(m_startM), "m"));
    direction = 1;
  }
  SetDirection(direction);
}

void InertialNavigator::SetDirection(int direction)
{
  if (direction == m_direction)
    return;
  // What the car has driven since the start was counted the other way.
  int const turned = direction - m_direction;
  double const heading = m_deadReckoning.GetHeading();
  if (m_offRoadCar)
    m_offRoadCar = Move(*m_offRoadCar, heading, turned * m_startM);
  else if (m_deadReckoning.IsReady() && !m_paused && !m_roadLost)
    m_deadReckoning.MoveTo(Move(m_deadReckoning.GetPosition(), heading, turned * m_startMarkM));
  m_direction = direction;
  if (direction > 0)
    return;
  // A turn of a car driving backwards is not a turn at a crossing, and the road doesn't bend as the car rotates.
  m_turning = false;
  m_bendMatcher.Reset();
}

void InertialNavigator::ResetDirection()
{
  m_direction = 1;
  m_standMs.reset();
  m_startMs.reset();
  m_standAccel.reset();
  m_otherWayMs.reset();
  m_startFelt = true;
  m_startM = 0;
  m_startMarkM = 0;
}

bool InertialNavigator::HasDrivenBackLately() const
{
  return m_lastBackMs && m_clock.NowMs() - *m_lastBackMs < kAccelAfterBackMs;
}

void InertialNavigator::ResetLag()
{
  m_lagSpeedMps.reset();
  m_lagPendingM = 0;
  m_accelAheadM = 0;
  m_phoneAheadM = 0;
}

void InertialNavigator::PauseByItself()
{
  LOG(LINFO, ("The car maneuvers", std::round(m_offRoadM), "m off the roads at up to",
              std::round(m_offRoadMaxSpeedMps * 3.6), "km/h, paused by itself, the mark waits at", *m_roadPoint));
  m_paused = true;
  m_pausedByItself = true;
  m_fastSinceMs.reset();
  m_turning = false;
  // The car is followed from where it is, its mark waits where it has left the roads.
  m_offRoadCar = m_deadReckoning.GetPosition();
  m_deadReckoning.MoveTo(*m_roadPoint);
}

void InertialNavigator::EndPauseByItself()
{
  if (!m_pausedByItself)
    return;
  LOG(LINFO, ("The pause set by itself is over after", std::round(m_offRoadM), "m off the roads"));
  m_paused = false;
  m_pausedByItself = false;
  m_offRoadCar.reset();
  m_roadCandidate.reset();
  m_fastSinceMs.reset();
}

bool InertialNavigator::IsRoadConfirmed(RoadPoint const & road, double headingDeg)
{
  double const bearing = Orient(road.m_bearingDeg, headingDeg);
  if (std::fabs(AngleDiff(bearing, headingDeg)) > DeadReckoning::kMaxSnapHeadingDiffDeg)
  {
    m_roadCandidate.reset();
    return false;
  }
  // Another road, or the car has turned: it is counted anew.
  if (!m_roadCandidate ||
      std::fabs(AngleDiff(bearing, m_roadCandidate->m_bearingDeg)) > kMaxRoadConfirmBearingDiffDeg ||
      std::fabs(AngleDiff(headingDeg, m_roadCandidate->m_headingDeg)) > kMaxRoadConfirmTurnDeg)
  {
    m_roadCandidate = RoadCandidate{bearing, m_pathM, headingDeg};
    return false;
  }
  return m_pathM - m_roadCandidate->m_pathM >= kRoadConfirmM;
}

void InertialNavigator::PutOnRoad(ms::LatLon const & point, double bearingDeg)
{
  EndPauseByItself();
  // Not a known place along the road: the accuracy is not what it is after a mark of the user.
  m_deadReckoning.MoveTo(point);
  SetRoadPoint(point);
  m_roadCandidate.reset();
  m_turning = false;
  m_onRoad = true;
  m_lastSnapResult = SnapResult::OnRoad;
  // The car drives along the road: what the gyroscope has counted through the maneuver is less sure.
  SetHeading(bearingDeg, HeadingSource::Road);
}

void InertialNavigator::CheckBackOnRoad()
{
  double const heading = m_deadReckoning.GetHeading();
  auto const road = m_roads.Snap(*m_offRoadCar, heading, kSnapRadiusM);
  if (!road)
  {
    m_lastSnapShiftM.reset();
    m_lastSnapBearingDiffDeg.reset();
    m_lastSnapResult = SnapResult::NoRoad;
    m_roadCandidate.reset();
    return;
  }
  double const shift = Distance(*m_offRoadCar, road->m_point);
  m_lastSnapShiftM = shift;
  m_lastSnapBearingDiffDeg = AngleDiff(heading, Orient(road->m_bearingDeg, heading));
  // A car driving backwards along a road is not driving along it yet.
  if (shift > kSnapRadiusM || m_direction < 0)
  {
    m_lastSnapResult = shift > kSnapRadiusM ? SnapResult::Far : SnapResult::Unconfirmed;
    m_roadCandidate.reset();
    return;
  }
  if (!IsRoadConfirmed(*road, heading))
  {
    m_lastSnapResult = std::fabs(*m_lastSnapBearingDiffDeg) > DeadReckoning::kMaxSnapHeadingDiffDeg
                           ? SnapResult::Angle
                           : SnapResult::Unconfirmed;
    return;
  }
  LOG(LINFO, ("The car drives along a road again", std::round(shift), "m aside, after", std::round(m_offRoadM),
              "m off the roads"));
  PutOnRoad(road->m_point, Orient(road->m_bearingDeg, heading));
}

std::optional<double> InertialNavigator::GetRoadCandidateM() const
{
  if (!m_roadCandidate)
    return {};
  return m_pathM - m_roadCandidate->m_pathM;
}

bool InertialNavigator::SetPaused(bool paused)
{
  // The pause set by itself is taken over by the user: resumed, or it is the pause of the user since now.
  bool const byItself = m_pausedByItself;
  if (m_paused == paused && !byItself)
    return true;
  LOG(LINFO, ("paused =", paused));
  double const heading = m_deadReckoning.GetHeading();
  // The car goes on from the road next to where it has maneuvered to.
  std::optional<RoadPoint> road;
  if (byItself && !paused && m_offRoadCar && m_deadReckoning.IsReady())
  {
    road = m_roads.Snap(*m_offRoadCar, heading, kMaxSnapRadiusM);
    // Going on from the mark, which waits where the car has left the roads, would throw away where the car is:
    // on October 11 it was thrown back so three times, and paused again 15 m later.
    if (!road)
    {
      LOG(LINFO, ("No road next to the car, the pause set by itself stays after", std::round(m_offRoadM),
                  "m off the roads"));
      return false;
    }
  }
  m_pausedByItself = false;
  m_offRoadCar.reset();
  m_roadCandidate.reset();
  m_paused = paused;
  m_fastSinceMs.reset();
  m_turning = false;
  if (paused || !m_deadReckoning.IsReady())
    return true;

  // The road next to where the mark stands.
  if (!road)
    road = m_roads.Snap(m_deadReckoning.GetPosition(), heading, kMaxSnapRadiusM);
  if (road)
    SetRoadPosition(road->m_point, Orient(road->m_bearingDeg, heading));
  return true;
}

void InertialNavigator::OnGyro(int64_t timestampNs, Vec3 const & gyro)
{
  if (m_phone)
    m_phone->OnGyro(timestampNs, gyro);
}

void InertialNavigator::OnAccel(int64_t timestampNs, Vec3 const & accel)
{
  if (!m_started)
    return;
  if (m_phone)
    m_phone->OnAccel(accel);
  // The time of the car speed, not of the sensor: they are compared.
  m_phoneAccelSpeed.OnAccel(m_clock.NowMs(), accel);
  // The log is to check on real trips what the accelerometer can tell.
  auto const line = m_accelLog.OnSample(timestampNs, accel[0], accel[1], accel[2]);
  if (line && m_moving)
    LOG(LINFO, ("ACC", *line));
}

void InertialNavigator::OnElm327Connected()
{
  if (m_phone && m_phone->GetElm327())
    m_phone->GetElm327()->OnConnected();
}

void InertialNavigator::OnElm327Bytes(std::string_view data)
{
  if (m_phone && m_phone->GetElm327())
    m_phone->GetElm327()->OnBytes(data);
}

void InertialNavigator::OnElm327Closed(std::string const & reason)
{
  if (m_phone && m_phone->GetElm327())
    m_phone->GetElm327()->OnClosed(reason);
}

void InertialNavigator::OnEsp32BleBytes(std::string_view bytes)
{
  if (m_esp32)
    m_esp32->OnBleBytes(bytes);
}

void InertialNavigator::OnEsp32BleState(BleState state)
{
  if (m_esp32)
    m_esp32->OnBleState(state);
}

BleState InertialNavigator::GetBleState() const
{
  return m_esp32 ? m_esp32->GetBleState() : BleState::Off;
}

std::optional<Esp32Source::DataRate> InertialNavigator::GetEsp32DataRate() const
{
  if (!m_esp32)
    return {};
  return m_esp32->GetDataRate();
}

void InertialNavigator::OnSpeed(int speedKmh, int64_t timeMs)
{
  m_speedKmh = speedKmh;
  m_speedTimeMs = timeMs;
  // The time the position is calculated with this speed since, not the time the car measured it at.
  int64_t const now = m_clock.NowMs();
  double const speedMps = speedKmh / 3.6 * GetSpeedScale(speedKmh);
  m_speedLag.OnCarSpeed(now, speedMps);
  CheckBoxAccel();
  for (AccelSpeed * accelSpeed : {&m_boxAccelSpeed, &m_phoneAccelSpeed})
  {
    accelSpeed->SetLag(m_speedLag.Get());
    // A car driving backwards slows down when its speed grows, and its start backward is not always told.
    accelSpeed->SetLearning(!m_paused && !HasDrivenBackLately() && speedKmh >= kAutoResumeSpeedKmh);
    accelSpeed->OnCarSpeed(now, speedMps);
  }
  m_moving = speedKmh > 0;
  if (!m_paused)
    return;
  if (speedKmh < kAutoResumeSpeedKmh)
  {
    m_fastSinceMs.reset();
    return;
  }
  if (!m_fastSinceMs)
    m_fastSinceMs = timeMs;
  if (timeMs - *m_fastSinceMs < kAutoResumeMs)
    return;
  if (!m_pausedByItself)
  {
    SetPaused(false);
    return;
  }
  // Nobody maneuvers that fast: the car is on a road. With no road going its way around, the calculation has gone
  // wrong, and the mark waits for the user.
  double const heading = m_deadReckoning.GetHeading();
  std::optional<RoadPoint> road;
  if (m_offRoadCar)
    road = m_roads.Snap(*m_offRoadCar, heading, kMaxSnapRadiusM);
  if (road &&
      std::fabs(AngleDiff(Orient(road->m_bearingDeg, heading), heading)) <= DeadReckoning::kMaxSnapHeadingDiffDeg)
  {
    LOG(LINFO, ("The car drives fast again, a road is", std::round(Distance(*m_offRoadCar, road->m_point)),
                "m aside, after", std::round(m_offRoadM), "m off the roads"));
    PutOnRoad(road->m_point, Orient(road->m_bearingDeg, heading));
  }
  else if (now - m_roadLostNotifiedMs >= kRoadLostReminderMs)
  {
    LOG(LINFO, ("The car drives fast, but no road goes its way around"));
    NotifyRoadLost();
  }
}

void InertialNavigator::OnMotion(double yawDeltaDeg, double dtSec, int64_t timestampNs)
{
  if (dtSec <= 0 || dtSec > kMaxMotionDtSec)
    return;

  // The car reports that it moves a second late: a second with a rotation is written too.
  auto const gyroLine = m_gyroLog.OnSample(timestampNs, yawDeltaDeg, IsSpeedFresh() ? m_speedKmh : -1);
  if (gyroLine && (m_moving || m_gyroLog.GetLineRotationDeg() >= kGyroLogMinRotationDeg))
    LOG(LINFO, ("GYRO", *gyroLine));

  bool const speedFresh = IsSpeedFresh();
  m_deadReckoning.SetSpeed(speedFresh ? m_speedKmh / 3.6 * GetSpeedScale(m_speedKmh) : 0);
  if (speedFresh)
    TrackDirection();
  else
    ResetDirection();
  bool const back = m_direction < 0;
  // The car stands where it is while it maneuvers or after it has left the roads.
  bool const stopped = m_paused || m_roadLost;
  int64_t const now = m_clock.NowMs();
  // A standing car can't turn, so the remaining gyroscope drift doesn't rotate the heading at stops. The car
  // reports 0 km/h standing, and 1 km/h, which is below kMovingSpeedMps, while it turns crawling in a jam.
  bool const calibrated = m_source && m_source->IsCalibrated() && speedFresh;
  bool const rotates = calibrated && m_deadReckoning.GetSpeed() > 0;
  double rotatedDeg = 0;
  if (rotates)
  {
    bool const trackTurn = !stopped && !back;
    if (!m_standRotation.empty())
    {
      // The car has started before it has reported that.
      double startDeg = 0;
      for (auto const & rotation : m_standRotation)
        startDeg += rotation.second;
      double const startSec = std::max(dtSec, (now - m_standRotation.front().first) / 1000.0);
      m_standRotation.clear();
      Rotate(startDeg, startSec, timestampNs, trackTurn);
      rotatedDeg += startDeg;
    }
    Rotate(yawDeltaDeg, dtSec, timestampNs, trackTurn);
    rotatedDeg += yawDeltaDeg;
  }
  else if (calibrated)
  {
    m_standRotation.emplace_back(now, yawDeltaDeg);
    int64_t const keepMs = static_cast<int64_t>(m_speedLag.Get() * 1000) + kStandMarginMs;
    while (now - m_standRotation.front().first > keepMs)
      m_standRotation.pop_front();
  }
  else
  {
    m_standRotation.clear();
  }
  if (speedFresh)
  {
    double const step = m_deadReckoning.GetSpeed() * dtSec;
    m_pathM += step;
    m_startM += step;
    // The car paused by itself is followed apart from its mark.
    if (m_pausedByItself && m_offRoadCar)
    {
      m_offRoadCar = Move(*m_offRoadCar, m_deadReckoning.GetHeading(), step * m_direction);
      m_offRoadM += step;
    }
  }
  if (back)
    m_lastBackMs = now;
  if (speedFresh && !stopped)
  {
    double const step = m_deadReckoning.GetSpeed() * dtSec;
    double const distance = DistanceWithLag(dtSec);
    m_startMarkM += distance;
    if (back)
    {
      m_deadReckoning.ReverseBy(distance);
    }
    else
    {
      m_deadReckoning.AdvanceBy(distance);
      m_bendMatcher.OnMotion(rotatedDeg, distance);
    }
    m_speedScale.OnDistance(step);
    if (IsReady())
      TrackOffRoad(step);
  }
  else
  {
    ResetLag();
  }

  if (m_roadLost && m_deadReckoning.GetSpeed() > kMovingSpeedMps && now - m_roadLostNotifiedMs >= kRoadLostReminderMs)
    NotifyRoadLost();
  if (IsReady() && m_pausedByItself && m_offRoadCar && m_deadReckoning.GetSpeed() > kMovingSpeedMps &&
      now - m_lastSnapMs >= kSnapIntervalMs)
  {
    m_lastSnapMs = now;
    CheckBackOnRoad();
  }
  if (IsReady() && !stopped && m_deadReckoning.GetSpeed() > kMovingSpeedMps && now - m_lastSnapMs >= kSnapIntervalMs)
  {
    m_lastSnapMs = now;
    SnapToRoad();
    if (m_onRoad && !m_turning && now - m_lastBendMatchMs >= kBendMatchIntervalMs)
    {
      m_lastBendMatchMs = now;
      MatchBend();
    }
  }
  if (IsReady() && now - m_lastOutputMs >= kOutputIntervalMs)
  {
    m_lastOutputMs = now;
    if (auto const location = GetLocation())
      m_listener.OnInertialLocation(*location);
  }
}

void InertialNavigator::Rotate(double yawDeltaDeg, double dtSec, int64_t timestampNs, bool trackTurn)
{
  if (trackTurn)
    TrackTurn(yawDeltaDeg, dtSec, timestampNs);
  m_deadReckoning.Rotate(yawDeltaDeg);
  m_turnSignChecker.OnGyro(yawDeltaDeg);
}

double InertialNavigator::DistanceWithLag(double dtSec)
{
  // The car reports its speed late: by the time the speed has changed, the car has already driven with the new one
  // for the time of the lag.
  double const speed = m_deadReckoning.GetSpeed();
  double const bySpeed = m_lagSpeedMps ? m_speedLag.Get() * (speed - *m_lagSpeedMps) : 0;
  m_lagSpeedMps = speed;
  // The accelerometer of the box knows how much faster the car is now than it reports. Without it, or after the
  // car drove backwards, the same distance is added when the car reports the new speed at last.
  int64_t const now = m_clock.NowMs();
  double const fadePart = std::min(1.0, dtSec / kAccelFadeSec);
  double ahead = bySpeed;
  std::optional<double> gain;
  if (!HasDrivenBackLately())
    gain = m_boxAccelSpeed.GetGain(now);
  if (gain)
  {
    ahead = *gain * dtSec;
    m_accelAheadM += ahead - bySpeed;
  }
  double const fade = m_accelAheadM * fadePart;
  m_accelAheadM -= fade;
  m_lagPendingM += ahead - fade;
  if (auto const phoneGain = m_phoneAccelSpeed.GetGain(now))
    m_phoneAheadM += *phoneGain * dtSec - bySpeed;
  m_phoneAheadM -= m_phoneAheadM * fadePart;
  double const distance = speed * dtSec + m_lagPendingM;
  m_lagPendingM = std::min(0.0, distance);
  return std::max(0.0, distance);
}

void InertialNavigator::OnSourceAccel(std::array<double, 3> const & accel)
{
  m_boxAccelSpeed.OnAccel(m_clock.NowMs(), accel);
}

std::optional<double> InertialNavigator::GetPhoneAccelAheadM() const
{
  if (!m_phoneAccelSpeed.IsUsable())
    return {};
  return m_phoneAheadM;
}

void InertialNavigator::SnapToRoad()
{
  auto const position = m_deadReckoning.GetPosition();
  double const heading = m_deadReckoning.GetHeading();
  auto const snapped = m_roads.Snap(position, heading, kSnapRadiusM);
  if (!snapped)
  {
    m_onRoad = false;
    m_lastSnapShiftM.reset();
    m_lastSnapBearingDiffDeg.reset();
    m_lastSnapResult = SnapResult::NoRoad;
    m_roadCandidate.reset();
    return;
  }
  // Roads are searched in a square, its corners are farther than the radius.
  double const shift = Distance(position, snapped->m_point);
  m_lastSnapShiftM = shift;
  m_lastSnapBearingDiffDeg = AngleDiff(heading, Orient(snapped->m_bearingDeg, heading));
  bool const otherWay = std::fabs(*m_lastSnapBearingDiffDeg) > DeadReckoning::kMaxSnapHeadingDiffDeg;
  // A maneuvering car is not thrown to a road far aside at once: it is put there after it has driven along it.
  if (shift > kMaxManeuverSnapM && shift <= kSnapRadiusM && m_deadReckoning.GetSpeed() <= kMaxManeuverSpeedMps)
  {
    m_onRoad = false;
    m_lastSnapResult = otherWay ? SnapResult::Angle : SnapResult::Unconfirmed;
    // A car driving backwards along a road is not driving along it yet.
    if (m_direction < 0)
    {
      m_roadCandidate.reset();
      return;
    }
    if (!IsRoadConfirmed(*snapped, heading))
      return;
    LOG(LINFO, ("The maneuvering car has driven along a road", std::round(shift), "m aside, it is put there"));
    PutOnRoad(snapped->m_point, Orient(snapped->m_bearingDeg, heading));
    return;
  }
  m_roadCandidate.reset();
  m_onRoad = shift <= kSnapRadiusM && m_deadReckoning.SnapToRoad(snapped->m_point, snapped->m_bearingDeg);
  m_lastSnapResult = m_onRoad ? SnapResult::OnRoad : shift > kSnapRadiusM ? SnapResult::Far : SnapResult::Angle;
  if (m_onRoad)
  {
    m_roadPoint = snapped->m_point;
    m_offRoadM = 0;
    m_offRoadMaxSpeedMps = 0;
  }
}

void InertialNavigator::MatchBend()
{
  // The calculated distance is wrong by tens of meters, and the road bends more often than the car turns at a
  // crossing: the car is moved along the road to where the bends of the road fit its rotation.
  auto const position = m_deadReckoning.GetPosition();
  double const heading = m_deadReckoning.GetHeading();
  double const maxShiftM = std::min(kMaxBendShiftM, std::max(kMinBendSearchM, m_deadReckoning.GetAccuracy()));
  auto const aheadM = m_bendMatcher.Match(m_roads, position, heading, maxShiftM);
  if (!aheadM)
    return;
  if (std::fabs(*aheadM) < kMinBendShiftM)
  {
    LOG(LINFO, ("Bend: the position fits the road, ahead by", std::round(*aheadM), "m"));
    // The place along the road is confirmed: the distance error is gone as after a move.
    m_deadReckoning.SetPosition(position);
    return;
  }
  if (m_deadReckoning.GetSpeed() < kMinBendMoveSpeedMps)
  {
    LOG(LINFO, ("Bend: ahead by", std::round(*aheadM), "m, but the car is too slow to trust it"));
    return;
  }
  auto const walked = RoadWalker::Walk(m_roads, position, heading, -*aheadM);
  // A crossing on the way: the road after it may be another one.
  if (!walked || std::fabs(walked->m_appliedM + *aheadM) > 1)
  {
    LOG(LINFO, ("Bend: ahead by", std::round(*aheadM), "m, but the road is not followed that far"));
    return;
  }
  LOG(LINFO, ("Bend: moved along the road by", std::round(-*aheadM), "m"));
  SetRoadPosition(walked->m_position, walked->m_bearingDeg);
}

void InertialNavigator::TrackTurn(double yawDeltaDeg, double dtSec, int64_t timestampNs)
{
  // When the turn is over, the car is moved to the crossing it has turned at.
  if (!IsReady())
    return;
  double const yawRateDeg = yawDeltaDeg / dtSec;
  m_turnRotationDeg += std::fabs(yawDeltaDeg);
  if (!m_turning)
  {
    if (std::fabs(yawRateDeg) < kTurnStartRateDeg)
      return;
    m_turning = true;
    m_turnFromBearing = m_deadReckoning.GetHeading();
    m_turnStart = m_deadReckoning.GetPosition();
    m_turnFromRoad = m_onRoad;
    m_turnCalmSinceNs.reset();
    m_turnRotationDeg = 0;
    m_turnStartNs = timestampNs;
    m_turnStartPathM = m_pathM;
    m_turnMinKmh = m_turnMaxKmh = m_speedKmh;
    return;
  }
  m_turnMinKmh = std::min(m_turnMinKmh, m_speedKmh);
  m_turnMaxKmh = std::max(m_turnMaxKmh, m_speedKmh);

  if (std::fabs(yawRateDeg) >= kTurnEndRateDeg)
  {
    m_turnCalmSinceNs.reset();
    return;
  }
  if (!m_turnCalmSinceNs)
  {
    m_turnCalmSinceNs = timestampNs;
    return;
  }
  if (timestampNs - *m_turnCalmSinceNs < kTurnEndCalmNs)
    return;

  m_turning = false;
  MatchTurn(timestampNs);
}

void InertialNavigator::MatchTurn(int64_t timestampNs)
{
  double const toBearing = m_deadReckoning.GetHeading();
  double const turnDeg = std::fabs(AngleDiff(toBearing, m_turnFromBearing));
  auto const corner = Move(m_turnStart, m_turnFromBearing, kTurnCornerAheadM);
  // For the trip log, to try other rules on real trips: the turn with the second the car went straight after it,
  // and the crossings around where it has started by the calculation.
  char buf[160];
  std::snprintf(buf, sizeof(buf), "from=%ld to=%ld rot=%ld sec=%.1f m=%ld kmh=%d-%d road=%d acc=%ld cr=",
                std::lround(m_turnFromBearing), std::lround(toBearing), std::lround(m_turnRotationDeg),
                (timestampNs - m_turnStartNs) / 1e9, std::lround(m_pathM - m_turnStartPathM), m_turnMinKmh,
                m_turnMaxKmh, m_turnFromRoad ? 1 : 0, std::lround(m_deadReckoning.GetAccuracy()));
  LOG(LINFO, ("TURN", buf + TurnMatcher::DescribeCrossings(m_roads, corner, m_turnFromBearing, toBearing)));
  if (m_turnRotationDeg - turnDeg > kMaxExtraTurnRotationDeg)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "rotating by",
                std::round(m_turnRotationDeg), "is a roundabout or a winding road"));
    return;
  }
  std::string whyNot;
  auto const crossing = TurnMatcher::FindCrossing(m_roads, corner, m_turnFromBearing, toBearing,
                                                  m_deadReckoning.GetAccuracy(), &whyNot);
  if (!crossing)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "is not at a crossing:",
                whyNot));
    return;
  }

  // The whole turn is moved to the crossing: the car has driven the same way after it.
  auto const & position = m_deadReckoning.GetPosition();
  ms::LatLon const moved(position.m_lat + crossing->m_lat - corner.m_lat,
                         position.m_lon + crossing->m_lon - corner.m_lon);
  auto const road = m_roads.Snap(moved, toBearing, kTurnSnapRadiusM);
  if (!road)
  {
    LOG(LINFO, ("No road after the turn at the crossing", *crossing));
    return;
  }
  if (m_turnFromRoad)
  {
    // The road the regular snapping keeps the car on, or takes it to in a moment.
    auto const here = m_roads.Snap(position, toBearing, kSnapRadiusM);
    if (here && Distance(position, here->m_point) <= kSnapRadiusM &&
        std::fabs(AngleDiff(Orient(here->m_bearingDeg, toBearing), toBearing)) <=
            DeadReckoning::kMaxSnapHeadingDiffDeg &&
        Distance(here->m_point, road->m_point) > kMaxFollowedTurnMoveM)
    {
      LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing),
                  "has followed the roads, the crossing", std::round(Distance(corner, *crossing)),
                  "m away would move the car by", std::round(Distance(here->m_point, road->m_point)), "m"));
      return;
    }
  }
  m_lastTurnShiftM = Distance(corner, *crossing);
  LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "is moved to the crossing by",
              std::round(*m_lastTurnShiftM), "m"));
  // The crossing is a known place, the distance error is gone.
  SetRoadPosition(road->m_point, Orient(road->m_bearingDeg, toBearing));
}

std::string DebugPrint(InertialNavigator::HeadingSource source)
{
  switch (source)
  {
  case InertialNavigator::HeadingSource::None: return "NONE";
  case InertialNavigator::HeadingSource::Gps: return "GPS";
  case InertialNavigator::HeadingSource::Road: return "ROAD";
  }
  UNREACHABLE();
}

std::string DebugPrint(InertialNavigator::SnapResult result)
{
  switch (result)
  {
  case InertialNavigator::SnapResult::None: return "NONE";
  case InertialNavigator::SnapResult::OnRoad: return "ROAD";
  case InertialNavigator::SnapResult::NoRoad: return "NOROAD";
  case InertialNavigator::SnapResult::Far: return "FAR";
  case InertialNavigator::SnapResult::Angle: return "ANGLE";
  case InertialNavigator::SnapResult::Unconfirmed: return "WAIT";
  }
  UNREACHABLE();
}
}  // namespace nogps

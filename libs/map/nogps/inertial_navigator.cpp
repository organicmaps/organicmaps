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
#include "base/math.hpp"

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
  m_roughSpeedTable.Deserialize(m_storage.Get<std::string>(Storage::kRoughSpeedTable, ""));
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
  DropOtherMarks();
  EndFollowingApart();
  m_deadReckoning.SetPosition(gps.m_position);
  SetRoadPoint(gps.m_position);
  m_turning = false;
  m_steadyRoad.reset();
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
  StartRoughStretch(0);
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
  EndFollowingApart();
  m_deadReckoning.SetPosition(position);
  SetRoadPoint(position);
  m_turning = false;
  LeaveRoundabout();
  SetHeading(bearingDeg, HeadingSource::Road);
  m_steadyRoad = SteadyRoad{bearingDeg, m_pathM};
  StartRoughStretch(0);
}

void InertialNavigator::SetMarkOnRoad(ms::LatLon const & position, double bearingDeg)
{
  // The rest of the turn the car is making: from where it looks to along the road, the way it was turning.
  std::optional<double> restOfTurn;
  if (m_turning && m_deadReckoning.HasHeading())
  {
    double const heading = m_deadReckoning.GetHeading();
    double const rest = AngleDiff(bearingDeg, heading);
    double const turned = AngleDiff(heading, m_turnFromBearing);
    if (rest * turned > 0 && std::fabs(rest) + std::fabs(turned) <= 180)
      restOfTurn = rest;
  }
  DropOtherMarks();
  SetRoadPosition(position, bearingDeg);
  // The user marks by eye, not as precisely as the roads tell.
  m_roughStretch.reset();
  if (!restOfTurn || std::fabs(*restOfTurn) < kMinMarkTurnDeg)
    return;
  m_markTurnLeftDeg = restOfTurn;
  m_markTurnCalmPathM.reset();
  LOG(LINFO, ("The car is marked while it turns: the rest of the turn,", std::round(*restOfTurn),
              "degrees, doesn't turn the mark away from its road"));
}

double InertialNavigator::AbsorbMarkTurn(double yawDeltaDeg, double dtSec)
{
  if (!m_markTurnLeftDeg)
    return yawDeltaDeg;
  // The turn is over when the car has driven straight for some meters.
  if (std::fabs(yawDeltaDeg / dtSec) >= kTurnEndRateDeg)
  {
    m_markTurnCalmPathM.reset();
  }
  else if (!m_markTurnCalmPathM)
  {
    m_markTurnCalmPathM = m_pathM;
  }
  else if (m_pathM - *m_markTurnCalmPathM >= kMarkTurnEndM)
  {
    m_markTurnLeftDeg.reset();
    return yawDeltaDeg;
  }
  // The car rotating the way the turn went on is making the rest of it. Another way, or farther than the road
  // goes, it turns away from the road.
  double & left = *m_markTurnLeftDeg;
  if (yawDeltaDeg * left <= 0)
    return yawDeltaDeg;
  double const absorbed = std::fabs(yawDeltaDeg) < std::fabs(left) ? yawDeltaDeg : left;
  left -= absorbed;
  return yawDeltaDeg - absorbed;
}

void InertialNavigator::OnPositionCorrected(ms::LatLon const & position, double bearingDeg, double appliedM)
{
  m_speedScale.OnCorrection(appliedM);
  m_storage.Set(Storage::kSpeedScale, m_speedScale.Get());
  LOG(LINFO, ("Corrected by", std::round(appliedM), "m, speed scale =", m_speedScale.Get()));
  SetRoadPosition(position, bearingDeg);
  // The user corrects by eye, not as precisely as the roads tell.
  m_roughStretch.reset();
}

double InertialNavigator::GetSpeedScale() const
{
  return GetSpeedScale(std::max(0, m_speedKmh));
}

double InertialNavigator::GetSpeedScale(int speedKmh) const
{
  if (auto const scale = m_speedTable.GetScale(speedKmh))
    return *scale;
  return m_roughSpeedTable.GetScale(speedKmh).value_or(m_speedScale.Get());
}

void InertialNavigator::StartRoughStretch(double errorM)
{
  // GPS measures the speeds better.
  if (m_speedTable.GetKnownRanges() > 0)
  {
    m_roughStretch.reset();
    return;
  }
  m_roughStretch = RoughStretch{};
  m_roughStretch->m_errorM = errorM;
  m_roughStretch->m_lastPosition = m_deadReckoning.GetPosition();
}

void InertialNavigator::TrackRoughRoad()
{
  if (!m_roughStretch)
    return;
  auto const position = m_deadReckoning.GetPosition();
  auto & stretch = *m_roughStretch;
  double const movedM = Distance(stretch.m_lastPosition, position);
  if (movedM > 0)
  {
    double const alongDeg = AngleDiff(Bearing(stretch.m_lastPosition, position), m_deadReckoning.GetHeading());
    stretch.m_roadM += movedM * std::cos(math::DegToRad(alongDeg));
  }
  stretch.m_lastPosition = position;
}

void InertialNavigator::TrackRoughStretch(double stepM)
{
  if (!m_roughStretch)
    return;
  m_roughStretch->m_calculatedTotalM += stepM;
  // As by GPS, slower speeds tell nothing.
  if (m_speedKmh < SpeedTable::kMinSpeedKmh)
    return;
  int const i = SpeedTable::Range(m_speedKmh);
  m_roughStretch->m_calculatedM[i] += stepM;
  m_roughStretch->m_carM[i] += stepM / GetSpeedScale(m_speedKmh);
}

bool InertialNavigator::LearnRoughSpeed(double aheadM, char const * where)
{
  if (!m_roughStretch || m_speedTable.GetKnownRanges() > 0)
    return true;
  TrackRoughRoad();
  auto const & stretch = *m_roughStretch;
  double const calculatedM = stretch.m_calculatedTotalM;
  // A bend is told again and again while the car drives through its window: the stretch goes on to the next one.
  if (calculatedM < kMinRoughStretchM)
    return false;
  // The way of the position along the roads, the snapping to them included, and the errors at both ends.
  double const realM = stretch.m_roadM - (aheadM - stretch.m_errorM);
  double const errorM = calculatedM - realM;
  if (std::fabs(errorM) > calculatedM * kMaxRoughStretchError)
    return true;
  double const real = realM / calculatedM;
  for (int i = 0; i < SpeedTable::kRanges; ++i)
    m_roughSpeedTable.OnDistance((i + 0.5) * SpeedTable::kRangeKmh, stretch.m_carM[i], stretch.m_calculatedM[i] * real);
  m_storage.Set(Storage::kRoughSpeedTable, m_roughSpeedTable.Serialize());
  LOG(LINFO, ("Rough speed table: the", where, "tells the calculated distance is", std::round(errorM), "m off in",
              std::round(calculatedM), "m, table =", m_roughSpeedTable.ToString()));
  return true;
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
              m_speedLag.IsMeasured(), "scale =", m_speedScale.Get(), "rough =", m_roughSpeedTable.ToString()));
}

void InertialNavigator::ClearSpeedCalibration()
{
  LOG(LINFO, ("Speed table was", m_speedTable.ToString(), "rough", m_roughSpeedTable.ToString()));
  m_speedTable.Clear();
  m_speedLag.Clear();
  // The next trip learns the rough table again, as the first one.
  m_roughSpeedTable.Clear();
  m_roughStretch.reset();
  m_storage.Set(Storage::kRoughSpeedTable, m_roughSpeedTable.Serialize());
  SaveSpeedTable();
  m_speedScale.Reset();
  m_storage.Set(Storage::kSpeedScale, 1.0);
}

void InertialNavigator::SetPosition(ms::LatLon const & position)
{
  DropOtherMarks();
  EndFollowingApart();
  m_steadyRoad.reset();
  m_roughStretch.reset();
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
  m_markTurnLeftDeg.reset();
  m_pathMatcher.SetHeading(headingDeg);
  m_steadyRoad.reset();
  // A heading set anew, e.g. the car reversed by the user, is not the way the stretch was driven.
  m_roughStretch.reset();
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
  // afterwards. Nor is a roundabout.
  if (m_onRoad || m_turning || m_atRoundabout || !m_roadPoint || m_roadLost)
    return;
  m_offRoadM += distanceM;
  m_offRoadMaxSpeedMps = std::max(m_offRoadMaxSpeedMps, m_deadReckoning.GetSpeed());
  // A car driving along a road aside is put on it in a moment, see SnapToRoad(): it is not followed apart meanwhile.
  if (m_offRoadMaxSpeedMps <= kMaxManeuverSpeedMps &&
      (m_offRoadM >= kLostRoadM || (m_offRoadM >= kManeuverOffRoadM && !m_roadCandidate)))
  {
    StartFollowingApart();
    return;
  }
  if (m_offRoadM < kLostRoadM)
    return;
  // The car is on another road the turn has fitted.
  for (size_t i = 0; i < m_otherMarks.size(); ++i)
  {
    if (m_otherMarks[i].m_onRoad)
    {
      TakeOtherMark(i, false /* keepShown */,
                    "the road of the shown one is lost after " + std::to_string(std::lround(m_offRoadM)) + " m");
      return;
    }
  }
  LOG(LINFO, ("The road is lost after", std::round(m_offRoadM), "m, stopping at", *m_roadPoint));
  m_roadLost = true;
  m_roughStretch.reset();
  m_deadReckoning.MoveTo(*m_roadPoint);
  NotifyRoadLost();
}

void InertialNavigator::CheckRoundabout()
{
  if (m_onRoad)
  {
    LeaveRoundabout();
    return;
  }
  auto const position = m_deadReckoning.GetPosition();
  if (m_roads.IsRoundaboutNear(position, kRoundaboutRadiusM))
    m_roundaboutPathM = m_pathM;
  bool const atRoundabout = m_roundaboutPathM && m_pathM - *m_roundaboutPathM <= kRoundaboutExitM;
  if (!atRoundabout)
  {
    LeaveRoundabout();
    return;
  }
  if (!m_atRoundabout)
  {
    LOG(LINFO, ("The car is off the roads at a roundabout after", std::round(m_offRoadM),
                "m off them, it is not lost until it has left it"));
  }
  m_atRoundabout = true;

  // The road the car leaves the roundabout by.
  double const heading = m_deadReckoning.GetHeading();
  auto const road = m_roads.Snap(position, heading, kMaxSnapRadiusM);
  if (!road || Distance(position, road->m_point) > kMaxSnapRadiusM)
  {
    m_exitCandidate.reset();
    return;
  }
  if (!IsRoadConfirmed(*road, heading, m_exitCandidate))
    return;
  LOG(LINFO, ("The car has left the roundabout along a road", std::round(Distance(position, road->m_point)),
              "m aside, it is put there"));
  PutOnRoad(road->m_point, Orient(road->m_bearingDeg, heading));
  LeaveRoundabout();
}

void InertialNavigator::LeaveRoundabout()
{
  m_atRoundabout = false;
  m_roundaboutPathM.reset();
  m_exitCandidate.reset();
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
  if (m_speedKmh >= kNotManeuveringSpeedKmh && direction < 0)
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
  else if (m_deadReckoning.IsReady() && !m_roadLost)
    m_deadReckoning.MoveTo(Move(m_deadReckoning.GetPosition(), heading, turned * m_startMarkM));
  for (auto & mark : m_otherMarks)
  {
    auto & reckoning = mark.m_deadReckoning;
    reckoning.MoveTo(Move(reckoning.GetPosition(), reckoning.GetHeading(), turned * m_startM));
  }
  m_direction = direction;
  if (direction > 0)
    return;
  // A turn of a car driving backwards is not a turn at a crossing, and the road doesn't bend as the car rotates.
  m_turning = false;
  m_bendMatcher.Reset();
  m_pathMatcher.Reset();
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

void InertialNavigator::StartFollowingApart()
{
  LOG(LINFO, ("The car maneuvers", std::round(m_offRoadM), "m off the roads at up to",
              std::round(m_offRoadMaxSpeedMps * 3.6), "km/h, it is followed apart, the mark waits at", *m_roadPoint));
  m_fastWithoutRoad = false;
  m_fastSinceMs.reset();
  m_turning = false;
  // The car is followed from where it is, its mark waits where it has left the roads.
  m_steadyRoad.reset();
  m_roughStretch.reset();
  m_offRoadCar = m_deadReckoning.GetPosition();
  m_deadReckoning.MoveTo(*m_roadPoint);
}

void InertialNavigator::EndFollowingApart()
{
  if (!m_offRoadCar)
    return;
  LOG(LINFO, ("The car is not followed apart any more after", std::round(m_offRoadM), "m off the roads"));
  m_offRoadCar.reset();
  m_roadCandidate.reset();
  m_fastSinceMs.reset();
}

bool InertialNavigator::IsRoadConfirmed(RoadPoint const & road, double headingDeg)
{
  return IsRoadConfirmed(road, headingDeg, m_roadCandidate);
}

bool InertialNavigator::IsRoadConfirmed(RoadPoint const & road, double headingDeg,
                                        std::optional<RoadCandidate> & candidate)
{
  double const bearing = Orient(road.m_bearingDeg, headingDeg);
  if (std::fabs(AngleDiff(bearing, headingDeg)) > DeadReckoning::kMaxSnapHeadingDiffDeg)
  {
    candidate.reset();
    return false;
  }
  // Another road, or the car has turned: it is counted anew.
  if (!candidate || std::fabs(AngleDiff(bearing, candidate->m_bearingDeg)) > kMaxRoadConfirmBearingDiffDeg ||
      std::fabs(AngleDiff(headingDeg, candidate->m_headingDeg)) > kMaxRoadConfirmTurnDeg)
  {
    candidate = RoadCandidate{bearing, m_pathM, headingDeg};
    return false;
  }
  return m_pathM - candidate->m_pathM >= kRoadConfirmM;
}

void InertialNavigator::PutOnRoad(ms::LatLon const & point, double bearingDeg)
{
  EndFollowingApart();
  // Not a known place along the road: the accuracy is not what it is after a mark of the user.
  m_deadReckoning.MoveTo(point);
  SetRoadPoint(point);
  m_roadCandidate.reset();
  m_turning = false;
  m_onRoad = true;
  m_lastSnapResult = SnapResult::OnRoad;
  // The car drives along the road: what the gyroscope has counted through the maneuver is less sure.
  SetHeading(bearingDeg, HeadingSource::Road);
  m_steadyRoad = SteadyRoad{bearingDeg, m_pathM};
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
    accelSpeed->SetLearning(!m_offRoadCar && !HasDrivenBackLately() && speedKmh >= kNotManeuveringSpeedKmh);
    accelSpeed->OnCarSpeed(now, speedMps);
  }
  m_moving = speedKmh > 0;
  if (!m_offRoadCar)
    return;
  if (speedKmh < kNotManeuveringSpeedKmh)
  {
    m_fastSinceMs.reset();
    return;
  }
  if (!m_fastSinceMs)
    m_fastSinceMs = timeMs;
  if (timeMs - *m_fastSinceMs < kNotManeuveringMs)
    return;
  // Nobody maneuvers that fast: the car is on a road. With no road going its way around, the calculation has gone
  // wrong, and the mark waits for the user.
  double const heading = m_deadReckoning.GetHeading();
  auto const road = m_roads.Snap(*m_offRoadCar, heading, kMaxSnapRadiusM);
  if (road &&
      std::fabs(AngleDiff(Orient(road->m_bearingDeg, heading), heading)) <= DeadReckoning::kMaxSnapHeadingDiffDeg)
  {
    LOG(LINFO, ("The car drives fast again, a road is", std::round(Distance(*m_offRoadCar, road->m_point)),
                "m aside, after", std::round(m_offRoadM), "m off the roads"));
    PutOnRoad(road->m_point, Orient(road->m_bearingDeg, heading));
  }
  else
  {
    m_fastWithoutRoad = true;
    if (now - m_roadLostNotifiedMs >= kRoadLostReminderMs)
    {
      LOG(LINFO, ("The car drives fast, but no road goes its way around"));
      NotifyRoadLost();
    }
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
  // The mark stands where it is while the car maneuvering off the roads is followed apart, and after the road is lost.
  bool const stopped = m_offRoadCar || m_roadLost;
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
      rotatedDeg += Rotate(startDeg, startSec, timestampNs, trackTurn);
    }
    rotatedDeg += Rotate(yawDeltaDeg, dtSec, timestampNs, trackTurn);
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
    if (m_offRoadCar)
    {
      m_offRoadCar = Move(*m_offRoadCar, m_deadReckoning.GetHeading(), step * m_direction);
      m_offRoadM += step;
    }
  }
  if (back)
    m_lastBackMs = now;
  // The other marks drive as the shown one does, or as the car drives while the shown one stands.
  double othersDistanceM = 0;
  double othersStepM = 0;
  if (speedFresh && !stopped)
  {
    double const step = m_deadReckoning.GetSpeed() * dtSec;
    double const distance = DistanceWithLag(dtSec);
    othersDistanceM = distance;
    othersStepM = step;
    m_startMarkM += distance;
    if (back)
    {
      m_deadReckoning.ReverseBy(distance);
      m_pathMatcher.Reset();
      m_roughStretch.reset();
    }
    else
    {
      m_deadReckoning.AdvanceBy(distance);
      if (m_turning)
        m_turnEnd = Move(m_turnEnd, m_deadReckoning.GetHeading(), distance);
      m_bendMatcher.OnMotion(rotatedDeg, distance);
      m_pathMatcher.OnMotion(rotatedDeg, distance, m_deadReckoning.GetHeading());
      TrackRoughStretch(step);
    }
    m_speedScale.OnDistance(step);
    if (IsReady())
      TrackOffRoad(step);
  }
  else
  {
    ResetLag();
    // The way of a car not followed is not known.
    m_pathMatcher.Reset();
    if (speedFresh)
      othersDistanceM = othersStepM = m_deadReckoning.GetSpeed() * dtSec;
  }
  MoveOtherMarks(rotatedDeg, othersDistanceM, othersStepM);

  if (m_roadLost && m_deadReckoning.GetSpeed() > kMovingSpeedMps && now - m_roadLostNotifiedMs >= kRoadLostReminderMs)
    NotifyRoadLost();
  if (IsReady() && m_offRoadCar && m_deadReckoning.GetSpeed() > kMovingSpeedMps &&
      now - m_lastSnapMs >= kSnapIntervalMs)
  {
    m_lastSnapMs = now;
    CheckBackOnRoad();
  }
  if (IsReady() && !stopped && m_deadReckoning.GetSpeed() > kMovingSpeedMps && now - m_lastSnapMs >= kSnapIntervalMs)
  {
    m_lastSnapMs = now;
    SnapToRoad();
    if (m_turning && !m_onRoad)
      m_turnLeftRoad = true;
    CheckRoundabout();
    if (m_onRoad && !m_turning && now - m_lastBendMatchMs >= kBendMatchIntervalMs)
    {
      m_lastBendMatchMs = now;
      MatchBend();
    }
  }
  TrackRoughRoad();
  if (IsReady() && !m_otherMarks.empty() && m_deadReckoning.GetSpeed() > kMovingSpeedMps &&
      now - m_lastOtherSnapMs >= kSnapIntervalMs)
  {
    m_lastOtherSnapMs = now;
    FollowOtherMarks();
  }
  if (IsReady() && !m_otherMarks.empty() && !m_turning && m_deadReckoning.GetSpeed() > kMovingSpeedMps &&
      now - m_lastMarksWayCheckMs >= kMarksWayCheckMs)
  {
    m_lastMarksWayCheckMs = now;
    CheckMarksByWay();
  }
  if (IsReady() && now - m_lastOutputMs >= kOutputIntervalMs)
  {
    m_lastOutputMs = now;
    if (auto const location = GetLocation())
      m_listener.OnInertialLocation(*location);
  }
}

double InertialNavigator::Rotate(double yawDeltaDeg, double dtSec, int64_t timestampNs, bool trackTurn)
{
  // The gyroscope is compared with GPS by all it has measured.
  m_turnSignChecker.OnGyro(yawDeltaDeg);
  yawDeltaDeg = AbsorbMarkTurn(yawDeltaDeg, dtSec);
  if (trackTurn)
    TrackTurn(yawDeltaDeg, dtSec, timestampNs);
  m_deadReckoning.Rotate(yawDeltaDeg);
  return yawDeltaDeg;
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

std::optional<RoadPoint> InertialNavigator::ChooseBranch(ms::LatLon const & position, double headingDeg)
{
  auto const roads = m_roads.SnapAll(position, headingDeg, kSnapRadiusM);
  bool forks = false;
  for (size_t i = 0; i < roads.size() && !forks; ++i)
  {
    for (size_t j = i + 1; j < roads.size() && !forks; ++j)
    {
      double const apart =
          std::fabs(AngleDiff(Orient(roads[i].m_bearingDeg, headingDeg), Orient(roads[j].m_bearingDeg, headingDeg)));
      forks = apart >= kMinForkDeg && apart <= kMaxForkDeg;
    }
  }
  if (!forks)
    return {};

  RoadPoint const * best = nullptr;
  double bestCost = 0;
  for (auto const & road : roads)
  {
    double const cost = Distance(position, road.m_point) +
                        kForkMetersPerDeg * std::fabs(AngleDiff(Orient(road.m_bearingDeg, headingDeg), headingDeg));
    if (best == nullptr || cost < bestCost)
    {
      best = &road;
      bestCost = cost;
    }
  }
  return *best;
}

void InertialNavigator::SnapToRoad()
{
  auto const position = m_deadReckoning.GetPosition();
  double const heading = m_deadReckoning.GetHeading();
  auto snapped = m_roads.Snap(position, heading, kSnapRadiusM);
  // A maneuvering car is among the driveways of a yard, they are not branches of its road.
  bool atFork = false;
  if (snapped && m_deadReckoning.GetSpeed() > kMaxManeuverSpeedMps)
  {
    if (auto const branch = ChooseBranch(position, heading))
    {
      atFork = true;
      if (Distance(branch->m_point, snapped->m_point) > kSameForkBranchM)
      {
        LOG(LINFO, ("FORK the car goes along the branch", std::round(Distance(position, branch->m_point)), "m aside,",
                    std::round(AngleDiff(Orient(branch->m_bearingDeg, heading), heading)),
                    "degrees off its heading, not along the one", std::round(Distance(position, snapped->m_point)),
                    "m aside,", std::round(AngleDiff(Orient(snapped->m_bearingDeg, heading), heading)), "degrees off"));
      }
      snapped = branch;
    }
  }
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
  m_onRoad = shift <= kSnapRadiusM && m_deadReckoning.SnapToRoad(snapped->m_point, snapped->m_bearingDeg, !atFork);
  m_lastSnapResult = m_onRoad ? SnapResult::OnRoad : shift > kSnapRadiusM ? SnapResult::Far : SnapResult::Angle;
  if (m_onRoad)
  {
    m_roadPoint = snapped->m_point;
    m_offRoadM = 0;
    m_offRoadMaxSpeedMps = 0;
    if (std::fabs(*m_lastSnapBearingDiffDeg) <= DeadReckoning::kMaxHeadingPullDiffDeg)
      m_steadyRoad = SteadyRoad{Orient(snapped->m_bearingDeg, heading), m_pathM};
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
    if (LearnRoughSpeed(*aheadM, "bend"))
      StartRoughStretch(*aheadM);
    return;
  }
  bool const slow = m_deadReckoning.GetSpeed() < kMinBendMoveSpeedMps;
  if (slow && !IsSlowBendConfirmed(*aheadM))
  {
    LOG(LINFO, ("Bend: ahead by", std::round(*aheadM), "m, but the car is too slow to trust it"));
    return;
  }
  // Through the crossings on the way: the bends were compared with the road going straight on through them, and
  // they fit it. A car back on a road after a parking lot was 32 m behind its mark by the bends, as GPS told
  // too, and was left there because of a crossing on these 32 m.
  auto const walked = RoadWalker::Walk(m_roads, position, heading, -*aheadM, false /* stopAtCrossing */);
  if (!walked || std::fabs(walked->m_appliedM + *aheadM) > 1)
  {
    LOG(LINFO, ("Bend: ahead by", std::round(*aheadM), "m, but the road is not followed that far"));
    return;
  }
  if (slow)
    LOG(LINFO, ("Bend: moved along the road by", std::round(-*aheadM), "m, told repeatedly to a slow car"));
  else
    LOG(LINFO, ("Bend: moved along the road by", std::round(-*aheadM), "m"));
  LearnRoughSpeed(*aheadM, "bend");
  SetRoadPosition(walked->m_position, walked->m_bearingDeg);
}

bool InertialNavigator::IsSlowBendConfirmed(double aheadM)
{
  double const distanceM = m_deadReckoning.GetDistanceSinceFix();
  if (!m_slowBends || m_slowBends->m_fixCount != m_deadReckoning.GetFixCount() ||
      std::fabs(aheadM - m_slowBends->m_firstAheadM) > kSlowBendAgreeM)
  {
    m_slowBends = SlowBends{m_deadReckoning.GetFixCount(), aheadM, distanceM, 1};
    return false;
  }
  ++m_slowBends->m_count;
  return m_slowBends->m_count >= kSlowBendCount && distanceM - m_slowBends->m_firstDistanceM >= kSlowBendDriveM;
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
    m_turnEnd = m_turnStart;
    m_turnFromRoad = m_onRoad;
    m_turnLeftRoad = false;
    m_turnSteadyFromBearing.reset();
    if (m_steadyRoad && m_pathM - m_steadyRoad->m_pathM <= kTurnFromRoadM && m_offRoadM <= kTurnFromRoadOffM)
      m_turnSteadyFromBearing = m_steadyRoad->m_bearingDeg;
    m_turnCalmSinceNs.reset();
    m_turnRotationDeg = 0;
    m_turnStartNs = timestampNs;
    m_turnStartPathM = m_pathM;
    m_turnMinKmh = m_turnMaxKmh = m_speedKmh;
    for (auto & mark : m_otherMarks)
    {
      mark.m_turnStart = mark.m_deadReckoning.GetPosition();
      mark.m_turnEnd = mark.m_turnStart;
      mark.m_turnFromBearing = mark.m_deadReckoning.GetHeading();
      mark.m_turnFromRoad = mark.m_onRoad;
    }
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
  // See kTurnFromRoadM.
  if (m_turnSteadyFromBearing && (m_turnFromRoad || m_turnMaxKmh / 3.6 > kMaxManeuverSpeedMps))
  {
    LOG(LINFO, ("The turn starts on the road the car was on, from", std::round(*m_turnSteadyFromBearing), "not",
                std::round(m_turnFromBearing), m_turnFromRoad ? "" : "off the roads"));
    m_turnFromRoad = true;
    m_turnFromBearing = *m_turnSteadyFromBearing;
  }
  double const toBearing = m_deadReckoning.GetHeading();
  double const turnDeg = std::fabs(AngleDiff(toBearing, m_turnFromBearing));
  auto const corner = Move(m_turnStart, m_turnFromBearing, kTurnCornerAheadM);
  // For the trip log, to try other rules on real trips: the turn with the second the car went straight after it,
  // and the crossings around where it has started by the calculation.
  char buf[160];
  std::snprintf(buf, sizeof(buf),
                "from=%ld to=%ld rot=%ld sec=%.1f m=%ld kmh=%d-%d road=%d acc=%ld cr=", std::lround(m_turnFromBearing),
                std::lround(toBearing), std::lround(m_turnRotationDeg), (timestampNs - m_turnStartNs) / 1e9,
                std::lround(m_pathM - m_turnStartPathM), m_turnMinKmh, m_turnMaxKmh, m_turnFromRoad ? 1 : 0,
                std::lround(m_deadReckoning.GetAccuracy()));
  LOG(LINFO, ("TURN", buf + TurnMatcher::DescribeCrossings(m_roads, corner, m_turnFromBearing, toBearing)));
  if (m_turnRotationDeg - turnDeg > kMaxExtraTurnRotationDeg)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "rotating by",
                std::round(m_turnRotationDeg), "is a roundabout or a winding road"));
    return;
  }
  // A car maneuvering off the roads turns where it is, in a yard or at a parking place, not at a crossing of the
  // streets around. Of 3 such turns moved to a crossing on October 11 two were moved away from the car, by 17 and
  // 28 m. Moving only to a single crossing that fits did not help: that day driven again, one of the two had a
  // single crossing left by a meter, and was moved 26 m away.
  bool const maneuvering = !m_turnFromRoad && m_turnMaxKmh / 3.6 <= kMaxManeuverSpeedMps;
  // A turn around has no crossing: the car turns on its road, or a ramp loops onto another one.
  if (!maneuvering && turnDeg > TurnMatcher::kMaxTurnDeg && MatchLoop(toBearing))
    return;

  auto const position = m_deadReckoning.GetPosition();
  auto const here = RoadGoing(position, toBearing);
  std::string whyNot;
  std::vector<ms::LatLon> crossings;
  auto const crossing =
      TurnMatcher::FindCrossing(m_roads, corner, m_turnFromBearing, toBearing, m_deadReckoning.GetAccuracy(), &whyNot,
                                !m_turnFromRoad /* onlyOne */, &crossings);
  // The whole turn is moved to a crossing: the car has driven the same way after it. The way of the turn is moved as
  // it was driven: the snapping may have taken the car to a road aside meanwhile, on October 16 to a street 38 m from
  // the right one, and the turn moved from there was not on the right one either.
  auto const movedTo = [&](ms::LatLon const & to) -> std::optional<TurnPlace>
  {
    ms::LatLon const moved(m_turnEnd.m_lat + to.m_lat - corner.m_lat, m_turnEnd.m_lon + to.m_lon - corner.m_lon);
    auto road = m_roads.Snap(moved, toBearing, kTurnSnapRadiusM);
    if (!road)
      return {};
    road->m_bearingDeg = Orient(road->m_bearingDeg, toBearing);
    return TurnPlace{*road, Distance(corner, to)};
  };
  auto const stay = here ? std::optional<TurnPlace>(TurnPlace{*here, {}}) : std::nullopt;

  TurnPlaces places;
  if (maneuvering)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing),
                "is not at a crossing: the car was maneuvering off the roads at up to", m_turnMaxKmh, "km/h"));
    places.m_chosen = stay;
  }
  else if (!crossing)
  {
    LOG(LINFO,
        ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "is not at a crossing:", whyNot));
    places.m_chosen = stay;
  }
  else if (auto const moved = movedTo(*crossing); !moved)
  {
    LOG(LINFO, ("No road after the turn at the crossing", *crossing));
    places.m_chosen = stay;
  }
  else if (m_turnFromRoad && !m_turnLeftRoad && here &&
           Distance(here->m_point, moved->m_road.m_point) > kMaxFollowedTurnMoveM)
  {
    // The road the regular snapping keeps the car on, or takes it to in a moment.
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing),
                "has followed the roads, the crossing", std::round(Distance(corner, *crossing)),
                "m away would move the car by", std::round(Distance(here->m_point, moved->m_road.m_point)), "m"));
    places.m_chosen = stay;
  }
  else
  {
    places.m_chosen = moved;
  }

  // The other places the turn fits, on other roads than the chosen one.
  std::vector<TurnPlace> fitting;
  if (stay)
    fitting.push_back(*stay);
  for (auto const & to : crossings)
    if (auto const moved = movedTo(to))
      fitting.push_back(*moved);
  for (auto const & place : fitting)
  {
    auto const sameRoad = [&](TurnPlace const & other)
    { return AsideM(place.m_road.m_point, other.m_road.m_point, toBearing) <= kOtherRoadAsideM; };
    if ((!places.m_chosen || !sameRoad(*places.m_chosen)) &&
        std::none_of(places.m_others.begin(), places.m_others.end(), sameRoad))
    {
      places.m_others.push_back(place);
    }
  }
  std::string byWay;
  if (CheckPlacesByWay(position, places, byWay))
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), byWay));

  // The other marks turn too, a mark whose road has no road the new way is not where the car is.
  bool const atCrossing = !maneuvering && turnDeg >= TurnMatcher::kMinTurnDeg && turnDeg <= TurnMatcher::kMaxTurnDeg;
  auto const otherWithRoad = TurnOtherMarks(toBearing, atCrossing);
  if (!places.m_chosen && atCrossing && otherWithRoad)
  {
    TakeOtherMark(*otherWithRoad, false /* keepShown */,
                  "the car has turned where the road of the shown one has no road that way");
  }
  else if (places.m_chosen && places.m_chosen->m_shiftM)
  {
    m_lastTurnShiftM = places.m_chosen->m_shiftM;
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "is moved to the crossing by",
                std::round(*m_lastTurnShiftM), "m"));
    // The crossing is a known place, the distance error is gone.
    SetRoadPosition(places.m_chosen->m_road.m_point, places.m_chosen->m_road.m_bearingDeg);
  }
  for (auto const & place : places.m_others)
    AddOtherMark(place.m_road, toBearing, place.m_shiftM ? "at a crossing the turn fits" : "where the turn has ended");
}

std::optional<RoadPoint> InertialNavigator::RoadGoing(ms::LatLon const & position, double bearingDeg)
{
  auto road = m_roads.Snap(position, bearingDeg, kSnapRadiusM);
  if (!road || Distance(position, road->m_point) > kSnapRadiusM)
    return {};
  road->m_bearingDeg = Orient(road->m_bearingDeg, bearingDeg);
  if (std::fabs(AngleDiff(road->m_bearingDeg, bearingDeg)) > DeadReckoning::kMaxSnapHeadingDiffDeg)
    return {};
  return road;
}

double InertialNavigator::AsideM(ms::LatLon const & a, ms::LatLon const & b, double bearingDeg)
{
  return std::fabs(Distance(a, b) * std::sin(math::DegToRad(AngleDiff(Bearing(a, b), bearingDeg))));
}

bool InertialNavigator::CheckPlacesByWay(ms::LatLon const & position, TurnPlaces & places, std::string & why)
{
  if (places.m_others.empty())
    return false;
  auto const chosenMisses =
      m_pathMatcher.CountMissesAt(m_roads, places.m_chosen ? places.m_chosen->m_road.m_point : position);
  if (!chosenMisses)
    return false;
  std::vector<int> misses;
  std::string list;
  for (auto const & place : places.m_others)
  {
    misses.push_back(m_pathMatcher.CountMissesAt(m_roads, place.m_road.m_point).value_or(*chosenMisses));
    list += (list.empty() ? "" : ",") + std::to_string(misses.back());
  }
  why = "WAY misses the roads at " + std::to_string(*chosenMisses) + " points where the rules put the car, " + list +
        " at the other places";
  auto const best = std::min_element(misses.begin(), misses.end());
  // The way lies on no roads, e.g. through a parking lot missing on the map: it tells nothing.
  if (std::min(*best, *chosenMisses) > kWayFitMisses)
    return true;
  if (*chosenMisses >= *best + kWayMoreMisses)
  {
    auto const index = static_cast<size_t>(best - misses.begin());
    auto const taken = places.m_others[index];
    if (places.m_chosen)
      places.m_others[index] = *places.m_chosen;
    else
      places.m_others.erase(places.m_others.begin() + index);
    places.m_chosen = taken;
    why += ", the car is put where the way lies on the roads";
  }
  return true;
}

void InertialNavigator::MoveOtherMarks(double rotatedDeg, double distanceM, double stepM)
{
  for (auto & mark : m_otherMarks)
  {
    auto & reckoning = mark.m_deadReckoning;
    reckoning.Rotate(rotatedDeg);
    if (m_direction < 0)
    {
      reckoning.ReverseBy(distanceM);
    }
    else
    {
      reckoning.AdvanceBy(distanceM);
      if (m_turning)
        mark.m_turnEnd = Move(mark.m_turnEnd, reckoning.GetHeading(), distanceM);
    }
    // A turn takes a car aside from both roads for a while, as the shown one.
    if (!mark.m_onRoad && !m_turning)
      mark.m_offRoadM += stepM;
  }
}

void InertialNavigator::FollowOtherMarks()
{
  auto const shown = m_offRoadCar ? *m_offRoadCar : m_deadReckoning.GetPosition();
  double const shownHeading = m_deadReckoning.GetHeading();
  for (size_t i = 0; i < m_otherMarks.size();)
  {
    auto & mark = m_otherMarks[i];
    auto & reckoning = mark.m_deadReckoning;
    auto const position = reckoning.GetPosition();
    auto const road = m_roads.Snap(position, reckoning.GetHeading(), kSnapRadiusM);
    mark.m_onRoad = road && Distance(position, road->m_point) <= kSnapRadiusM &&
                    reckoning.SnapToRoad(road->m_point, road->m_bearingDeg);
    if (mark.m_onRoad)
      mark.m_offRoadM = 0;
    std::string dropped;
    if (!mark.m_onRoad && mark.m_offRoadM >= kLostRoadM)
    {
      dropped = "its road has ended " + std::to_string(std::lround(mark.m_offRoadM)) + " m behind";
    }
    else if (Distance(reckoning.GetPosition(), shown) <= kSameMarkM &&
             std::fabs(AngleDiff(reckoning.GetHeading(), shownHeading)) <= DeadReckoning::kMaxSnapHeadingDiffDeg)
    {
      dropped = "it is where the shown one is";
    }
    else if (mark.m_onRoad && m_onRoad && !m_offRoadCar && !m_turning &&
             std::fabs(AngleDiff(reckoning.GetHeading(), shownHeading)) <= DeadReckoning::kMaxSnapHeadingDiffDeg &&
             IsOnShownRoad(reckoning.GetPosition()))
    {
      dropped = "it is on the road of the shown one " +
                std::to_string(std::lround(Distance(reckoning.GetPosition(), shown))) + " m away";
    }
    else
    {
      for (size_t j = 0; j < i && dropped.empty(); ++j)
        if (Distance(reckoning.GetPosition(), m_otherMarks[j].m_deadReckoning.GetPosition()) <= kSameMarkM)
          dropped = "it is where another one is";
    }
    if (dropped.empty())
    {
      ++i;
      continue;
    }
    LOG(LINFO, ("Other mark dropped:", dropped, ", left", m_otherMarks.size() - 1));
    m_otherMarks.erase(m_otherMarks.begin() + i);
  }
  // The shown mark stands lost while the car may be on another road.
  if (m_roadLost)
  {
    for (size_t i = 0; i < m_otherMarks.size(); ++i)
    {
      if (m_otherMarks[i].m_onRoad)
      {
        TakeOtherMark(i, false /* keepShown */, "the shown one stands lost");
        break;
      }
    }
  }
}

bool InertialNavigator::IsOnShownRoad(ms::LatLon const & position)
{
  auto const shown = m_deadReckoning.GetPosition();
  double const distanceM = Distance(shown, position);
  if (distanceM > kSameRoadCheckM)
    return false;
  double const heading = m_deadReckoning.GetHeading();
  double const aheadM = distanceM * std::cos(math::DegToRad(AngleDiff(Bearing(shown, position), heading)));
  // Through the crossings: the road going straight on is the same road.
  auto const walked = RoadWalker::Walk(m_roads, shown, heading, aheadM, false /* stopAtCrossing */);
  return walked && Distance(walked->m_position, position) <= kSameRoadAsideM;
}

void InertialNavigator::CheckMarksByWay()
{
  auto const shownMisses = m_pathMatcher.CountMissesAt(m_roads, m_deadReckoning.GetPosition());
  if (!shownMisses)
    return;
  std::vector<int> misses;
  for (auto const & mark : m_otherMarks)
    misses.push_back(m_pathMatcher.CountMissesAt(m_roads, mark.m_deadReckoning.GetPosition()).value_or(*shownMisses));
  auto const best = std::min_element(misses.begin(), misses.end());
  if (std::min(*best, *shownMisses) > kWayFitMisses)
    return;
  if (*shownMisses >= *best + kWayMoreMisses)
  {
    auto const index = static_cast<size_t>(best - misses.begin());
    TakeOtherMark(index, !m_roadLost /* keepShown */,
                  "WAY misses the roads at " + std::to_string(*shownMisses) + " points where it was shown, at " +
                      std::to_string(*best) + " there");
  }
}

std::optional<size_t> InertialNavigator::TurnOtherMarks(double toBearingDeg, bool atCrossing)
{
  std::optional<size_t> withRoad;
  for (size_t i = 0; i < m_otherMarks.size();)
  {
    auto & mark = m_otherMarks[i];
    auto & reckoning = mark.m_deadReckoning;
    auto const position = reckoning.GetPosition();
    auto const here = RoadGoing(position, toBearingDeg);
    std::optional<RoadPoint> road;
    if (atCrossing)
    {
      auto const corner = Move(mark.m_turnStart, mark.m_turnFromBearing, kTurnCornerAheadM);
      if (auto const crossing = TurnMatcher::FindCrossing(m_roads, corner, mark.m_turnFromBearing, toBearingDeg,
                                                          reckoning.GetAccuracy(), nullptr, !mark.m_turnFromRoad))
      {
        ms::LatLon const moved(mark.m_turnEnd.m_lat + crossing->m_lat - corner.m_lat,
                               mark.m_turnEnd.m_lon + crossing->m_lon - corner.m_lon);
        road = m_roads.Snap(moved, toBearingDeg, kTurnSnapRadiusM);
        // As the shown one: a mark that has followed the roads is where they have taken it.
        if (road && mark.m_turnFromRoad && here && Distance(here->m_point, road->m_point) > kMaxFollowedTurnMoveM)
          road.reset();
      }
    }
    if (road)
    {
      reckoning.MoveTo(road->m_point);
      reckoning.SetHeading(Orient(road->m_bearingDeg, toBearingDeg));
      mark.m_onRoad = true;
      mark.m_offRoadM = 0;
    }
    else if (!here && atCrossing)
    {
      LOG(LINFO, ("Other mark dropped: the car has turned where its road has no road that way, left",
                  m_otherMarks.size() - 1));
      m_otherMarks.erase(m_otherMarks.begin() + i);
      continue;
    }
    if (!withRoad && (road || here))
      withRoad = i;
    ++i;
  }
  return withRoad;
}

void InertialNavigator::AddOtherMark(RoadPoint const & road, double headingDeg, std::string const & why)
{
  auto const sameRoad = [&](ms::LatLon const & other)
  { return AsideM(road.m_point, other, headingDeg) <= kOtherRoadAsideM; };
  if (sameRoad(m_deadReckoning.GetPosition()) ||
      std::any_of(m_otherMarks.begin(), m_otherMarks.end(),
                  [&](OtherMark const & mark) { return sameRoad(mark.m_deadReckoning.GetPosition()); }))
  {
    return;
  }
  if (m_otherMarks.size() + 1 >= kMaxMarks)
  {
    LOG(LINFO, ("Other mark not added", why, ": there are", kMaxMarks, "marks already"));
    return;
  }
  OtherMark mark;
  mark.m_deadReckoning = m_deadReckoning;
  mark.m_deadReckoning.MoveTo(road.m_point);
  mark.m_deadReckoning.SetHeading(Orient(road.m_bearingDeg, headingDeg));
  mark.m_turnStart = mark.m_turnEnd = road.m_point;
  mark.m_turnFromBearing = mark.m_deadReckoning.GetHeading();
  m_otherMarks.push_back(mark);
  LOG(LINFO, ("Other mark added", why, ",", std::round(AsideM(road.m_point, m_deadReckoning.GetPosition(), headingDeg)),
              "m aside of the shown one at", road.m_point, ", marks", m_otherMarks.size() + 1));
}

void InertialNavigator::TakeOtherMark(size_t index, bool keepShown, std::string const & why)
{
  CHECK_LESS(index, m_otherMarks.size(), ());
  auto const taken = m_otherMarks[index];
  m_otherMarks.erase(m_otherMarks.begin() + index);
  if (keepShown)
  {
    OtherMark shown;
    shown.m_deadReckoning = m_deadReckoning;
    if (m_offRoadCar)
      shown.m_deadReckoning.MoveTo(*m_offRoadCar);
    shown.m_onRoad = m_onRoad && !m_offRoadCar;
    shown.m_offRoadM = shown.m_onRoad ? 0 : m_offRoadM;
    shown.m_turnStart = m_turnStart;
    shown.m_turnEnd = m_turnEnd;
    shown.m_turnFromBearing = m_turnFromBearing;
    shown.m_turnFromRoad = m_turnFromRoad;
    m_otherMarks.push_back(shown);
  }
  EndFollowingApart();
  m_deadReckoning = taken.m_deadReckoning;
  m_headingSource = HeadingSource::Road;
  m_pathMatcher.SetHeading(m_deadReckoning.GetHeading());
  SetRoadPoint(m_deadReckoning.GetPosition());
  m_onRoad = taken.m_onRoad;
  m_offRoadM = taken.m_offRoadM;
  m_turnStart = taken.m_turnStart;
  m_turnEnd = taken.m_turnEnd;
  m_turnFromBearing = taken.m_turnFromBearing;
  m_turnFromRoad = taken.m_turnFromRoad;
  m_roadCandidate.reset();
  m_slowBends.reset();
  m_markTurnLeftDeg.reset();
  m_steadyRoad.reset();
  m_turnSteadyFromBearing.reset();
  m_roughStretch.reset();
  LeaveRoundabout();
  LOG(LINFO, ("The car is shown on another road it may be on:", why, ", marks", m_otherMarks.size() + 1, "at",
              m_deadReckoning.GetPosition()));
  m_listener.OnMarkSwitched();
}

std::vector<ms::LatLon> InertialNavigator::GetOtherMarkPositions() const
{
  std::vector<ms::LatLon> positions;
  for (auto const & mark : m_otherMarks)
    positions.push_back(mark.m_deadReckoning.GetPosition());
  return positions;
}

bool InertialNavigator::SwitchMark()
{
  if (m_otherMarks.empty())
    return false;
  TakeOtherMark(0, !m_roadLost /* keepShown */, "the user has switched the marks");
  return true;
}

void InertialNavigator::DropOtherMarks()
{
  if (m_otherMarks.empty())
    return;
  LOG(LINFO, ("Other marks dropped, the place of the car is known:", m_otherMarks.size()));
  m_otherMarks.clear();
}

bool InertialNavigator::MatchLoop(double toBearingDeg)
{
  auto const position = m_deadReckoning.GetPosition();
  std::string why;
  auto const road = m_pathMatcher.Match(m_roads, position, &why);
  if (!road)
  {
    LOG(LINFO, ("Loop: not moved,", why));
    return false;
  }
  LOG(LINFO, ("Loop: the way driven lies on the roads", std::round(Distance(position, road->m_point)),
              "m away, moved there,", why));
  SetRoadPosition(road->m_point, Orient(road->m_bearingDeg, toBearingDeg));
  return true;
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

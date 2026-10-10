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
  // The phone may be put into the holder another way, the box may be another one.
  m_boxAccelSpeed.Reset();
  m_phoneAccelSpeed.Reset();
  m_accelAheadM = 0;
  m_phoneAheadM = 0;
  m_delegate.StartMotionSensors(!esp32 /* gyroscope */);
}

void InertialNavigator::Stop()
{
  if (!m_started)
    return;
  m_started = false;
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

void InertialNavigator::PauseByItself()
{
  LOG(LINFO, ("The car maneuvers", std::round(m_offRoadM), "m off the roads, paused by itself, the mark waits at",
              *m_roadPoint));
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
  // The car drives along the road: what the gyroscope has counted through the maneuver is less sure.
  SetHeading(bearingDeg, HeadingSource::Road);
}

void InertialNavigator::CheckBackOnRoad()
{
  double const heading = m_deadReckoning.GetHeading();
  auto const road = m_roads.Snap(*m_offRoadCar, heading, kSnapRadiusM);
  if (!road || Distance(*m_offRoadCar, road->m_point) > kSnapRadiusM)
  {
    m_roadCandidate.reset();
    return;
  }
  if (!IsRoadConfirmed(*road, heading))
    return;
  LOG(LINFO, ("The car drives along a road again", std::round(Distance(*m_offRoadCar, road->m_point)),
              "m aside, after", std::round(m_offRoadM), "m off the roads"));
  PutOnRoad(road->m_point, Orient(road->m_bearingDeg, heading));
}

void InertialNavigator::SetPaused(bool paused)
{
  // The pause set by itself is taken over by the user: resumed, or it is the pause of the user since now.
  bool const byItself = m_pausedByItself;
  if (m_paused == paused && !byItself)
    return;
  LOG(LINFO, ("paused =", paused));
  std::optional<ms::LatLon> car;
  if (byItself)
    car = m_offRoadCar;
  m_pausedByItself = false;
  m_offRoadCar.reset();
  m_roadCandidate.reset();
  m_paused = paused;
  m_fastSinceMs.reset();
  m_turning = false;
  if (paused || !m_deadReckoning.IsReady())
    return;

  double const heading = m_deadReckoning.GetHeading();
  // The road next to where the car has maneuvered to, or to where its mark stands.
  std::optional<RoadPoint> road;
  if (car)
    road = m_roads.Snap(*car, heading, kMaxSnapRadiusM);
  if (!road)
    road = m_roads.Snap(m_deadReckoning.GetPosition(), heading, kMaxSnapRadiusM);
  if (road)
    SetRoadPosition(road->m_point, Orient(road->m_bearingDeg, heading));
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
  for (AccelSpeed * accelSpeed : {&m_boxAccelSpeed, &m_phoneAccelSpeed})
  {
    accelSpeed->SetLag(m_speedLag.Get());
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
  auto const gyroLine = m_gyroLog.OnSample(timestampNs, yawDeltaDeg);
  if (gyroLine && (m_moving || m_gyroLog.GetLineRotationDeg() >= kGyroLogMinRotationDeg))
    LOG(LINFO, ("GYRO", *gyroLine));

  bool const speedFresh = IsSpeedFresh();
  m_deadReckoning.SetSpeed(speedFresh ? m_speedKmh / 3.6 * GetSpeedScale(m_speedKmh) : 0);
  // The car stands where it is while it maneuvers or after it has left the roads.
  bool const stopped = m_paused || m_roadLost;
  // A standing car can't turn, so the remaining gyroscope drift doesn't rotate the heading at stops. The car
  // reports 0 km/h standing, and 1 km/h, which is below kMovingSpeedMps, while it turns crawling in a jam.
  bool const rotates = m_source && m_source->IsCalibrated() && speedFresh && m_deadReckoning.GetSpeed() > 0;
  if (rotates)
  {
    if (!stopped)
      TrackTurn(yawDeltaDeg, dtSec, timestampNs);
    m_deadReckoning.Rotate(yawDeltaDeg);
    m_turnSignChecker.OnGyro(yawDeltaDeg);
  }
  if (speedFresh)
  {
    double const step = m_deadReckoning.GetSpeed() * dtSec;
    m_pathM += step;
    // The car paused by itself is followed apart from its mark.
    if (m_pausedByItself && m_offRoadCar)
    {
      m_offRoadCar = Move(*m_offRoadCar, m_deadReckoning.GetHeading(), step);
      m_offRoadM += step;
    }
  }
  if (speedFresh && !stopped)
  {
    double const distance = DistanceWithLag(dtSec);
    m_deadReckoning.AdvanceBy(distance);
    m_bendMatcher.OnMotion(rotates ? yawDeltaDeg : 0, distance);
    m_speedScale.OnDistance(m_deadReckoning.GetSpeed() * dtSec);
    if (IsReady())
      TrackOffRoad(m_deadReckoning.GetSpeed() * dtSec);
  }
  else
  {
    m_lagSpeedMps.reset();
    m_lagPendingM = 0;
    m_accelAheadM = 0;
    m_phoneAheadM = 0;
  }

  int64_t const now = m_clock.NowMs();
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

double InertialNavigator::DistanceWithLag(double dtSec)
{
  // The car reports its speed late: by the time the speed has changed, the car has already driven with the new one
  // for the time of the lag.
  double const speed = m_deadReckoning.GetSpeed();
  double const bySpeed = m_lagSpeedMps ? m_speedLag.Get() * (speed - *m_lagSpeedMps) : 0;
  m_lagSpeedMps = speed;
  // The accelerometer of the box knows how much faster the car is now than it reports. Without it the same
  // distance is added when the car reports the new speed at last.
  int64_t const now = m_clock.NowMs();
  double const fadePart = std::min(1.0, dtSec / kAccelFadeSec);
  double ahead = bySpeed;
  if (auto const gain = m_boxAccelSpeed.GetGain(now))
  {
    ahead = *gain * dtSec;
    m_accelAheadM += ahead - bySpeed;
  }
  double const fade = m_accelAheadM * fadePart;
  m_accelAheadM -= fade;
  m_lagPendingM += ahead - fade;
  if (auto const gain = m_phoneAccelSpeed.GetGain(now))
    m_phoneAheadM += *gain * dtSec - bySpeed;
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
    m_roadCandidate.reset();
    return;
  }
  // Roads are searched in a square, its corners are farther than the radius.
  double const shift = Distance(position, snapped->m_point);
  m_lastSnapShiftM = shift;
  // A maneuvering car is not thrown to a road far aside at once: it is put there after it has driven along it.
  if (shift > kMaxManeuverSnapM && shift <= kSnapRadiusM && m_deadReckoning.GetSpeed() <= kMaxManeuverSpeedMps)
  {
    m_onRoad = false;
    if (!IsRoadConfirmed(*snapped, heading))
      return;
    LOG(LINFO, ("The maneuvering car has driven along a road", std::round(shift), "m aside, it is put there"));
    PutOnRoad(snapped->m_point, Orient(snapped->m_bearingDeg, heading));
    return;
  }
  m_roadCandidate.reset();
  m_onRoad = shift <= kSnapRadiusM && m_deadReckoning.SnapToRoad(snapped->m_point, snapped->m_bearingDeg);
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
    return;
  }

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
  MatchTurn();
}

void InertialNavigator::MatchTurn()
{
  double const toBearing = m_deadReckoning.GetHeading();
  double const turnDeg = std::fabs(AngleDiff(toBearing, m_turnFromBearing));
  if (m_turnRotationDeg - turnDeg > kMaxExtraTurnRotationDeg)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "rotating by",
                std::round(m_turnRotationDeg), "is a roundabout or a winding road"));
    return;
  }
  auto const corner = Move(m_turnStart, m_turnFromBearing, kTurnCornerAheadM);
  auto const crossing =
      TurnMatcher::FindCrossing(m_roads, corner, m_turnFromBearing, toBearing, m_deadReckoning.GetAccuracy());
  if (!crossing)
  {
    LOG(LINFO, ("Turn from", std::round(m_turnFromBearing), "to", std::round(toBearing), "is not at a crossing"));
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
}  // namespace nogps

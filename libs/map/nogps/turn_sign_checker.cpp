#include "map/nogps/turn_sign_checker.hpp"

#include "map/nogps/geo.hpp"

#include "base/assert.hpp"

#include <cmath>

namespace nogps
{
void TurnSignChecker::OnGyro(double deltaDeg)
{
  if (m_lastBearingDeg)
    m_gyroTurnDeg += deltaDeg;
}

std::optional<TurnSignChecker::Result> TurnSignChecker::OnGpsBearing(double bearingDeg, int64_t timeMs)
{
  if (!m_lastBearingDeg || timeMs - m_lastGpsMs > kMaxGpsGapMs)
  {
    Restart(bearingDeg, timeMs);
    return {};
  }
  double const step = AngleDiff(bearingDeg, *m_lastBearingDeg);
  m_lastBearingDeg = bearingDeg;
  m_lastGpsMs = timeMs;
  m_gpsTurnDeg += step;
  if (std::fabs(step) > kMaxStraightStepDeg)
    return {};

  // Driving straight: a turn has ended, or the gyroscope drift is dropped.
  Result const old = m_result;
  if (std::fabs(m_gpsTurnDeg) >= kMinTurnDeg && std::fabs(m_gyroTurnDeg) >= kMinTurnDeg / 2)
  {
    if ((m_gpsTurnDeg > 0) == (m_gyroTurnDeg > 0))
      ++m_sameTurns;
    else
      ++m_oppositeTurns;
    if (m_oppositeTurns >= kTurnsToDecide && m_oppositeTurns >= kTurnsToDecide * m_sameTurns)
      m_result = Result::Reversed;
    else if (m_sameTurns >= kTurnsToDecide && m_sameTurns >= kTurnsToDecide * m_oppositeTurns)
      m_result = Result::Ok;
  }
  m_gpsTurnDeg = 0;
  m_gyroTurnDeg = 0;
  if (m_result != old)
    return m_result;
  return {};
}

void TurnSignChecker::Restart(double bearingDeg, int64_t timeMs)
{
  m_lastBearingDeg = bearingDeg;
  m_lastGpsMs = timeMs;
  m_gpsTurnDeg = 0;
  m_gyroTurnDeg = 0;
}

std::string DebugPrint(TurnSignChecker::Result result)
{
  switch (result)
  {
  case TurnSignChecker::Result::Unknown: return "Unknown";
  case TurnSignChecker::Result::Ok: return "Ok";
  case TurnSignChecker::Result::Reversed: return "Reversed";
  }
  UNREACHABLE();
}
}  // namespace nogps

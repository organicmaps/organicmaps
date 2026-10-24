#include "map/nogps/dead_reckoning.hpp"

#include "map/nogps/geo.hpp"

#include <algorithm>
#include <cmath>

namespace nogps
{
void DeadReckoning::SetPosition(ms::LatLon const & position)
{
  m_hasPosition = true;
  m_position = position;
  m_distanceSinceFixM = 0;
  ++m_fixCount;
}

void DeadReckoning::MoveTo(ms::LatLon const & position)
{
  m_position = position;
}

void DeadReckoning::SetHeading(double headingDeg)
{
  m_hasHeading = true;
  m_headingDeg = Normalize(headingDeg);
  m_isAside = false;
  m_trustedUntilM = 0;
}

void DeadReckoning::TrustRoad()
{
  m_trustedUntilM = m_odometerM + kMaxAsideOnRoadM;
}

void DeadReckoning::SetSpeed(double speedMps)
{
  m_speedMps = std::max(0.0, speedMps);
}

void DeadReckoning::Rotate(double deltaDeg)
{
  if (m_hasHeading)
    m_headingDeg = Normalize(m_headingDeg + deltaDeg);
}

void DeadReckoning::Advance(double dtSec)
{
  if (!IsReady() || dtSec <= 0)
    return;
  AdvanceBy(m_speedMps * dtSec);
}

void DeadReckoning::AdvanceBy(double distanceM)
{
  if (!IsReady() || distanceM <= 0)
    return;
  m_position = Move(m_position, m_headingDeg, distanceM);
  m_distanceSinceFixM += distanceM;
  m_odometerM += distanceM;
}

void DeadReckoning::ReverseBy(double distanceM)
{
  if (!IsReady() || distanceM <= 0)
    return;
  m_position = Move(m_position, m_headingDeg, -distanceM);
  m_distanceSinceFixM += distanceM;
  m_odometerM += distanceM;
}

bool DeadReckoning::SnapToRoad(ms::LatLon const & road, double roadBearingDeg, bool pullHeading)
{
  if (!IsReady())
    return false;
  double diff = AngleDiff(roadBearingDeg, m_headingDeg);
  if (std::fabs(diff) > 90)
    diff = AngleDiff(roadBearingDeg + 180, m_headingDeg);
  if (std::fabs(diff) > kMaxSnapHeadingDiffDeg)
    return false;

  m_position = road;
  if (!pullHeading)
    return true;
  double const sign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
  if (std::fabs(diff) <= kMaxHeadingPullDiffDeg)
  {
    m_isAside = false;
  }
  else if (!m_isAside || sign != m_asideSign)
  {
    m_isAside = true;
    m_asideSinceM = m_odometerM;
    m_asideSign = sign;
  }
  if (m_odometerM < m_trustedUntilM || !m_isAside || m_odometerM - m_asideSinceM >= kMaxAsideOnRoadM)
    m_headingDeg = Normalize(m_headingDeg + diff * kSnapHeadingWeight);
  return true;
}
}  // namespace nogps

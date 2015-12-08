#pragma once

#include "geometry/point2d.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
class AStarProgress
{
public:
  AStarProgress(float startValue, float stopValue)
      : m_startValue(startValue), m_stopValue(stopValue) {}

  void Initialize(m2::PointD const & startPoint, m2::PointD const & finalPoint)
  {
    m_startPoint = startPoint;
    m_finalPoint = finalPoint;
    m_initialDistance = MercatorBounds::DistanceOnEarth(startPoint, finalPoint);
    m_forwardDistance = m_initialDistance;
    m_backwardDistance = m_initialDistance;
    m_lastValue = m_startValue;
  }

  float GetProgressForDirectedAlgo(m2::PointD const & point)
  {
    return CheckConstraints(
        1.0 - MercatorBounds::DistanceOnEarth(point, m_finalPoint) / m_initialDistance);
  }

  float GetProgressForBidirectedAlgo(m2::PointD const & point, m2::PointD const & target)
  {
    if (target == m_finalPoint)
      m_forwardDistance = MercatorBounds::DistanceOnEarth(point, target);
    else if (target == m_startPoint)
      m_backwardDistance = MercatorBounds::DistanceOnEarth(point, target);
    else
      ASSERT(false, ());
    return CheckConstraints(2.0 - (m_forwardDistance + m_backwardDistance) / m_initialDistance);
  }

  float GetLastValue() const { return m_lastValue; }

private:
  float CheckConstraints(float const & roadPart)
  {
    float mappedValue = m_startValue + (m_stopValue - m_startValue) * roadPart;
    mappedValue = my::clamp(mappedValue, m_startValue, m_stopValue);
    if (mappedValue > m_lastValue)
      m_lastValue = mappedValue;
    return m_lastValue;
  }

  float m_forwardDistance;
  float m_backwardDistance;
  float m_initialDistance;
  float m_lastValue;

  m2::PointD m_startPoint;
  m2::PointD m_finalPoint;

  float const m_startValue;
  float const m_stopValue;
};

}  //  namespace routing

#pragma once

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <iterator>
#include <list>

namespace routing
{
class AStarSubProgress
{
public:
  AStarSubProgress(m2::PointD const & start, m2::PointD const & finish, double contributionCoef)
    : m_contributionCoef(contributionCoef), m_startPoint(start), m_finalPoint(finish)
  {
    ASSERT_NOT_EQUAL(m_contributionCoef, 0.0, ());

    m_fullDistance = MercatorBounds::DistanceOnEarth(start, finish);
    m_forwardDistance = m_fullDistance;
    m_backwardDistance = m_fullDistance;
  }

  explicit AStarSubProgress(double contributionCoef)
      : m_contributionCoef(contributionCoef)
  {
    ASSERT_NOT_EQUAL(m_contributionCoef, 0.0, ());
  }

  double UpdateProgress(m2::PointD const & current, m2::PointD const & end)
  {
    ASSERT_NOT_EQUAL(m_fullDistance, 0.0, ());

    double dist = MercatorBounds::DistanceOnEarth(current, end);
    double & toUpdate = end == m_finalPoint ? m_forwardDistance : m_backwardDistance;

    toUpdate = std::min(toUpdate, dist);

    double part = 2.0 - (m_forwardDistance + m_backwardDistance) / m_fullDistance;
    part = base::clamp(part, 0.0, 1.0);
    double newProgress =  m_contributionCoef * part;

    m_currentProgress = std::max(newProgress, m_currentProgress);

    return m_currentProgress;
  }

  double UpdateProgress(double subSubProgressValue)
  {
    return m_currentProgress + m_contributionCoef * subSubProgressValue;
  }

  void Flush(double progress)
  {
    m_currentProgress += m_contributionCoef * progress;
  }

  double GetMaxContribution() const { return m_contributionCoef; }

private:

  double m_currentProgress = 0.0;

  double m_contributionCoef = 0.0;

  double m_fullDistance = 0.0;

  double m_forwardDistance = 0.0;
  double m_backwardDistance = 0.0;

  m2::PointD m_startPoint;
  m2::PointD m_finalPoint;
};

class AStarProgress
{
public:

  AStarProgress()
  {
    m_subProgresses.emplace_back(AStarSubProgress(1.0));
  }

  void AppendSubProgress(AStarSubProgress const & subProgress)
  {
    m_subProgresses.emplace_back(subProgress);
  }

  void EraseLastSubProgress()
  {
    ASSERT(m_subProgresses.begin() != m_subProgresses.end(), ());
    ASSERT(m_subProgresses.begin() != std::prev(m_subProgresses.end()), ());

    auto prevLast = std::prev(std::prev(m_subProgresses.end()));
    prevLast->Flush(m_subProgresses.back().GetMaxContribution());

    CHECK(!m_subProgresses.empty(), ());
    auto last = std::prev(m_subProgresses.end());
    m_subProgresses.erase(last);
  }

  double UpdateProgress(m2::PointD const & current, m2::PointD const & end)
  {
    return UpdateProgressImpl(m_subProgresses.begin(), current, end) * 100.0;
  }

  double GetLastValue() const { return m_lastValue * 100.0; }

private:

  using ListItem = std::list<AStarSubProgress>::iterator;

  double UpdateProgressImpl(ListItem subProgress, m2::PointD const & current,
                            m2::PointD const & end)
  {
    if (std::next(subProgress) != m_subProgresses.end())
      return subProgress->UpdateProgress(UpdateProgressImpl(std::next(subProgress), current, end));

    return subProgress->UpdateProgress(current, end);
  }

  double m_lastValue = 0.0;

  std::list<AStarSubProgress> m_subProgresses;
};
}  //  namespace routing

#include "routing/base/astar_progress.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <iterator>

namespace routing
{
// AStarSubProgress ----------------------------------------------------------------------

AStarSubProgress::AStarSubProgress(ms::LatLon const & start, ms::LatLon const & finish,
                                   double contributionCoef)
  : m_contributionCoef(contributionCoef), m_startPoint(start), m_finishPoint(finish)
{
  ASSERT_GREATER(m_contributionCoef, 0.0, ());

  m_fullDistance = ms::DistanceOnEarth(start, finish);
  m_forwardDistance = m_fullDistance;
  m_backwardDistance = m_fullDistance;
}

AStarSubProgress::AStarSubProgress(double contributionCoef)
    : m_contributionCoef(contributionCoef)
{
  ASSERT_NOT_EQUAL(m_contributionCoef, 0.0, ());
}

double AStarSubProgress::UpdateProgress(ms::LatLon const & current, ms::LatLon const & finish)
{
  ASSERT_NOT_EQUAL(m_fullDistance, 0.0, ());

  double const dist = ms::DistanceOnEarth(current, finish);
  double & toUpdate = finish == m_finishPoint ? m_forwardDistance : m_backwardDistance;

  toUpdate = std::min(toUpdate, dist);

  double part = 2.0 - (m_forwardDistance + m_backwardDistance) / m_fullDistance;
  part = base::Clamp(part, 0.0, 1.0);
  double const newProgress =  m_contributionCoef * part;

  m_currentProgress = std::max(newProgress, m_currentProgress);
  return m_currentProgress;
}

double AStarSubProgress::UpdateProgress(double subSubProgressValue)
{
  return m_currentProgress + m_contributionCoef * subSubProgressValue;
}

void AStarSubProgress::Flush(double progress)
{
  m_currentProgress += m_contributionCoef * progress;
}

double AStarSubProgress::GetMaxContribution() const { return m_contributionCoef; }

// AStarProgress -------------------------------------------------------------------------

// static
double const AStarProgress::kMaxPercent = 99.0;

AStarProgress::AStarProgress()
{
  m_subProgresses.emplace_back(AStarSubProgress(kMaxPercent / 100.0));
}

AStarProgress::~AStarProgress()
{
  CHECK(std::next(m_subProgresses.begin()) == m_subProgresses.end(), ());
}

void AStarProgress::AppendSubProgress(AStarSubProgress const & subProgress)
{
  m_subProgresses.emplace_back(subProgress);
}

void AStarProgress::DropLastSubProgress()
{
  CHECK(!m_subProgresses.empty(), ());
  auto const last = std::prev(m_subProgresses.end());
  m_subProgresses.erase(last);
}

void AStarProgress::PushAndDropLastSubProgress()
{
  CHECK(m_subProgresses.begin() != m_subProgresses.end(), ());
  CHECK(m_subProgresses.begin() != std::prev(m_subProgresses.end()), ());

  auto prevLast = std::prev(std::prev(m_subProgresses.end()));
  prevLast->Flush(m_subProgresses.back().GetMaxContribution());

  DropLastSubProgress();
}

double AStarProgress::UpdateProgress(ms::LatLon const & current, ms::LatLon const & end)
{
  double const newProgress = UpdateProgressImpl(m_subProgresses.begin(), current, end) * 100.0;
  m_lastPercentValue = std::max(m_lastPercentValue, newProgress);

  ASSERT(m_lastPercentValue < kMaxPercent ||
             base::AlmostEqualAbs(m_lastPercentValue, kMaxPercent, 1e-5 /* eps */),
         (m_lastPercentValue, kMaxPercent));

  m_lastPercentValue = std::min(m_lastPercentValue, kMaxPercent);
  return m_lastPercentValue;
}

double AStarProgress::GetLastPercent() const { return m_lastPercentValue; }

double AStarProgress::UpdateProgressImpl(ListItem subProgress, ms::LatLon const & current,
                                         ms::LatLon const & end)
{
  if (std::next(subProgress) != m_subProgresses.end())
    return subProgress->UpdateProgress(UpdateProgressImpl(std::next(subProgress), current, end));

  return subProgress->UpdateProgress(current, end);
}
}  // namespace routing

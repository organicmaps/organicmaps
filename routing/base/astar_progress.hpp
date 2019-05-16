#pragma once

#include "geometry/point2d.hpp"

#include <list>

namespace routing
{
class AStarSubProgress
{
public:
  AStarSubProgress(m2::PointD const & start, m2::PointD const & finish, double contributionCoef);

  explicit AStarSubProgress(double contributionCoef);

  double UpdateProgress(m2::PointD const & current, m2::PointD const & finish);
  /// \brief Some |AStarSubProgress| could have their own subProgresses (next item in
  /// AStarProgress::m_subProgresses after current), in order to return true progress
  /// back to the upper level of subProgresses, we should do progress backpropagation of
  /// subProgress of current subProgress - |subSubProgressValue|
  double UpdateProgress(double subSubProgressValue);

  void Flush(double progress);
  double GetMaxContribution() const;

private:
  double m_currentProgress = 0.0;
  double m_contributionCoef = 0.0;
  double m_fullDistance = 0.0;
  double m_forwardDistance = 0.0;
  double m_backwardDistance = 0.0;

  m2::PointD m_startPoint;
  m2::PointD m_finishPoint;
};

class AStarProgress
{
public:
  static double const kMaxPercent;

  AStarProgress();
  ~AStarProgress();

  double GetLastPercent() const;
  void PushAndDropLastSubProgress();
  void DropLastSubProgress();
  void AppendSubProgress(AStarSubProgress const & subProgress);
  double UpdateProgress(m2::PointD const & current, m2::PointD const & end);

private:
  using ListItem = std::list<AStarSubProgress>::iterator;

  double UpdateProgressImpl(ListItem subProgress, m2::PointD const & current,
                            m2::PointD const & end);

  // This value is in range: [0, 1].
  double m_lastPercentValue = 0.0;

  std::list<AStarSubProgress> m_subProgresses;
};
}  //  namespace routing

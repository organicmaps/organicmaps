#pragma once

#include "geometry/latlon.hpp"

#include <list>

namespace routing
{
class AStarSubProgress
{
public:
  AStarSubProgress(ms::LatLon const & start, ms::LatLon const & finish, double contributionCoef);

  explicit AStarSubProgress(double contributionCoef);

  double UpdateProgress(ms::LatLon const & current, ms::LatLon const & finish);
  /// \brief Some |AStarSubProgress| could have their own subProgresses (next item in
  /// AStarProgress::m_subProgresses after current), in order to return true progress
  /// back to the upper level of subProgresses, we should do progress backpropagation of
  /// subProgress of current subProgress - |subSubProgressValue|
  double UpdateProgress(double subSubProgressValue);

  void Flush(double progress);
  double GetMaxContribution() const;

private:
  static double CalcDistance(ms::LatLon const & from, ms::LatLon const & to);

  double m_currentProgress = 0.0;
  double m_contributionCoef = 0.0;
  double m_fullDistance = 0.0;
  double m_forwardDistance = 0.0;
  double m_backwardDistance = 0.0;

  ms::LatLon m_startPoint;
  ms::LatLon m_finishPoint;
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
  double UpdateProgress(ms::LatLon const & current, ms::LatLon const & end);

private:
  using ListItem = std::list<AStarSubProgress>::iterator;

  double UpdateProgressImpl(ListItem subProgress, ms::LatLon const & current, ms::LatLon const & end);

  // This value is in range: [0, 1].
  double m_lastPercentValue = 0.0;

  std::list<AStarSubProgress> m_subProgresses;
};
}  //  namespace routing

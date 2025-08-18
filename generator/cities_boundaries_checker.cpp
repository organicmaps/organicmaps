#include "generator/cities_boundaries_checker.hpp"

#include "geometry/rect2d.hpp"

namespace generator
{
CitiesBoundariesChecker::CitiesBoundariesChecker(CitiesBoundaries const & citiesBoundaries)
{
  for (auto const & cb : citiesBoundaries)
    m_tree.Add(cb, cb.m_bbox.ToRect());
}

bool CitiesBoundariesChecker::InCity(m2::PointD const & point) const
{
  bool result = false;
  m_tree.ForEachInRect(m2::RectD(point, point), [&](indexer::CityBoundary const & cityBoundary)
  {
    if (result)
      return;

    result = cityBoundary.HasPoint(point);
  });

  return result;
}
}  // namespace generator

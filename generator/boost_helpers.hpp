#pragma once

#include "generator/feature_builder.hpp"

#include <iterator>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomma"
#endif  // #ifdef __clang__
#include <boost/geometry.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // #ifdef __clang__

namespace generator
{
namespace boost_helpers
{

template <typename BoostGeometry, typename FbGeometry>
void FillBoostGeometry(BoostGeometry & geometry, FbGeometry const & fbGeometry)
{
  using BoostPoint = typename boost::geometry::point_type<BoostGeometry>::type;

  geometry.clear();
  geometry.reserve(fbGeometry.size());
  for (auto const & p : fbGeometry)
    boost::geometry::append(geometry, BoostPoint{p.x, p.y});
}

template <typename BoostPolygon>
void FillPolygon(BoostPolygon & polygon, feature::FeatureBuilder const & fb)
{
  auto const & fbGeometry = fb.GetGeometry();
  CHECK(!fbGeometry.empty(), ());

  polygon.clear();
  FillBoostGeometry(polygon.outer(), *fbGeometry.begin());
  polygon.inners().resize(fbGeometry.size() - 1);

  size_t i = 0;

  for (auto it = std::next(fbGeometry.begin()); it != fbGeometry.end(); ++it)
    FillBoostGeometry(polygon.inners()[i++], *it);

  boost::geometry::correct(polygon);
}
}  // namespace boost_helpers
}  // namespace generator

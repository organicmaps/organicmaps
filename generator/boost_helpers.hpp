#pragma once

#include "generator/feature_builder.hpp"

#include <iterator>

#include <boost/geometry.hpp>

namespace generator
{
namespace boost_helpers
{
using BoostPoint = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;

template <typename BoostPoint, typename BoostGeometry, typename FbGeometry>
void FillBoostGeometry(BoostGeometry & geometry, FbGeometry const & fbGeometry)
{
  geometry.reserve(fbGeometry.size());
  for (auto const & p : fbGeometry)
    boost::geometry::append(geometry, BoostPoint{p.x, p.y});
}

template <typename BoostPolygon>
void FillPolygon(BoostPolygon & polygon, feature::FeatureBuilder const & fb)
{
  using BoostPoint = typename BoostPolygon::point_type;
  auto const & fbGeometry = fb.GetGeometry();
  CHECK(!fbGeometry.empty(), ());
  auto it = std::begin(fbGeometry);
  FillBoostGeometry<BoostPoint>(polygon.outer(), *it);
  polygon.inners().resize(fbGeometry.size() - 1);
  int i = 0;
  ++it;
  for (; it != std::end(fbGeometry); ++it)
    FillBoostGeometry<BoostPoint>(polygon.inners()[i++], *it);

  boost::geometry::correct(polygon);
}
}  // namespace boost_helpers
}  // namespace generator

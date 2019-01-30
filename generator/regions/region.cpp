#include "generator/regions/region.hpp"

#include "generator/boost_helpers.hpp"
#include "generator/regions/city.hpp"
#include "generator/regions/collector_region_info.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <numeric>

#include <boost/geometry.hpp>

namespace generator
{
namespace regions
{

BoostPolygon MakePolygonWithRadius(BoostPoint const & point, double radius, size_t numPoints  = 16)
{
  boost::geometry::strategy::buffer::point_circle point_strategy(numPoints);
  boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(radius);

  static boost::geometry::strategy::buffer::join_round const join_strategy;
  static boost::geometry::strategy::buffer::end_round const end_strategy;
  static boost::geometry::strategy::buffer::side_straight const side_strategy;

  boost::geometry::model::multi_polygon<BoostPolygon> result;
  boost::geometry::buffer(point, result, distance_strategy, side_strategy, join_strategy,
                          end_strategy, point_strategy);
  CHECK_EQUAL(result.size(), 1, ());
  return std::move(result.front());
}
Region::Region(FeatureBuilder1 const & fb, RegionDataProxy const & rd)
  : RegionWithName(fb.GetParams().name)
  , RegionWithData(rd)
  , m_polygon(std::make_shared<BoostPolygon>())
{
  FillPolygon(fb);
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
}

Region::Region(City const & city)
  : RegionWithName(city.GetMultilangName())
  , RegionWithData(city.GetRegionData())
  , m_polygon(std::make_shared<BoostPolygon>())
{
  auto const radius = GetRadiusByPlaceType(city.GetPlaceType());
  *m_polygon = MakePolygonWithRadius(city.GetCenter(), radius);
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
}

// static
double Region::GetRadiusByPlaceType(PlaceType place)
{
  // Based on average radiuses of OSM place polygons.
  switch (place)
  {
  case PlaceType::City:
    return 0.078;
  case PlaceType::Town:
    return 0.033;
  case PlaceType::Village:
    return 0.013;
  case PlaceType::Hamlet:
    return 0.0067;
  case PlaceType::Suburb:
    return 0.016;
  case PlaceType::Neighbourhood:
  case PlaceType::IsolatedDwelling:
    return 0.0035;
  case PlaceType::Locality:
  case PlaceType::Unknown:
    UNREACHABLE();
  }
  UNREACHABLE();
}

void Region::DeletePolygon()
{
  m_polygon = nullptr;
}

void Region::FillPolygon(FeatureBuilder1 const & fb)
{
  CHECK(m_polygon, ());
  boost_helpers::FillPolygon(*m_polygon, fb);
}

bool Region::IsCountry() const
{
  static auto const kAdminLevelCountry = AdminLevel::Two;
  return !HasPlaceType() && GetAdminLevel() == kAdminLevelCountry;
}

bool Region::IsLocality() const
{
  return HasPlaceType();
}

bool Region::Contains(Region const & smaller) const
{
  CHECK(m_polygon, ());
  CHECK(smaller.m_polygon, ());

  return boost::geometry::covered_by(smaller.m_rect, m_rect) &&
      boost::geometry::covered_by(*smaller.m_polygon, *m_polygon);
}

double Region::CalculateOverlapPercentage(Region const & other) const
{
  CHECK(m_polygon, ());
  CHECK(other.m_polygon, ());

  if (!boost::geometry::intersects(other.m_rect, m_rect))
    return 0.0;

  std::vector<BoostPolygon> coll;
  boost::geometry::intersection(*other.m_polygon, *m_polygon, coll);
  auto const min = std::min(boost::geometry::area(*other.m_polygon),
                            boost::geometry::area(*m_polygon));
  auto const binOp = [](double x, BoostPolygon const & y) { return x + boost::geometry::area(y); };
  auto const sum = std::accumulate(std::begin(coll), std::end(coll), 0., binOp);
  return (sum / min) * 100;
}

bool Region::ContainsRect(Region const & smaller) const
{
  return boost::geometry::covered_by(smaller.m_rect, m_rect);
}

BoostPoint Region::GetCenter() const
{
  BoostPoint p;
  boost::geometry::centroid(m_rect, p);
  return p;
}

bool Region::Contains(City const & cityPoint) const
{
  CHECK(m_polygon, ());

  return Contains(cityPoint.GetCenter());
}

bool Region::Contains(BoostPoint const & point) const
{
  CHECK(m_polygon, ());

  return boost::geometry::covered_by(point, m_rect) &&
      boost::geometry::covered_by(point, *m_polygon);
}

bool FeatureCityPointToRegion(RegionInfo const & regionInfo, FeatureBuilder1 & feature)
{
  if (!feature.IsPoint())
    return false;

  auto const center = feature.GetKeyPoint();
  BoostPolygon polygon;
  auto info = regionInfo.Get(feature.GetMostGenericOsmId());
  if (!info.HasPlaceType())
    return false;

  auto const placeType = info.GetPlaceType();
  if (placeType == PlaceType::Locality || placeType == PlaceType::Unknown)
    return false;

  auto const radius = Region::GetRadiusByPlaceType(placeType);
  polygon = MakePolygonWithRadius({center.x, center.y}, radius);
  auto const & outer = polygon.outer();
  FeatureBuilder1::PointSeq seq;
  std::transform(std::begin(outer), std::end(outer), std::back_inserter(seq), [](BoostPoint const & p) {
    return m2::PointD(p.get<0>(), p.get<1>());
  });
  feature.ResetGeometry();
  feature.AddPolygon(seq);
  feature.SetAreaAddHoles({});
  feature.SetRank(0);
  return true;
}
}  // namespace regions
}  // namespace generator

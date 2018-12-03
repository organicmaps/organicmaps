#include "generator/regions/region.hpp"

#include "generator/boost_helpers.hpp"
#include "generator/regions/city.hpp"
#include "generator/regions/collector_region_info.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <numeric>

#include <boost/geometry.hpp>

namespace generator
{
namespace regions
{
Region::Region(FeatureBuilder1 const & fb, RegionDataProxy const & rd)
  : RegionWithName(fb.GetParams().name),
    RegionWithData(rd),
    m_polygon(std::make_shared<BoostPolygon>())
{
  FillPolygon(fb);
  auto rect = fb.GetLimitRect();
  m_rect = BoostRect({{rect.minX(), rect.minY()}, {rect.maxX(), rect.maxY()}});
  m_area = boost::geometry::area(*m_polygon);
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

bool Region::Contains(Region const & smaller) const
{
  CHECK(m_polygon, ());
  CHECK(smaller.m_polygon, ());

  return boost::geometry::covered_by(*smaller.m_polygon, *m_polygon);
}

double Region::CalculateOverlapPercentage(Region const & other) const
{
  CHECK(m_polygon, ());
  CHECK(other.m_polygon, ());

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

  return boost::geometry::covered_by(cityPoint.GetCenter(), *m_polygon);
}

void Region::SetInfo(City const & cityPoint)
{
  SetStringUtf8MultilangName(cityPoint.GetStringUtf8MultilangName());
  SetAdminLevel(cityPoint.GetAdminLevel());
  SetPlaceType(cityPoint.GetPlaceType());
}
}  // namespace regions
}  // namespace generator

#include "generator/regions/region.hpp"

#include "generator/boost_helpers.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/place_point.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <numeric>

#include <boost/geometry.hpp>

using namespace feature;

namespace generator
{
namespace regions
{
BoostPolygon MakePolygonWithRadius(BoostPoint const & point, double radius, size_t numPoints = 16)
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

Region::Region(FeatureBuilder const & fb, RegionDataProxy const & rd)
  : RegionWithName(fb.GetParams().name)
  , RegionWithData(rd)
  , m_polygon(std::make_shared<BoostPolygon>())
{
  FillPolygon(fb);
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
}

Region::Region(PlacePoint const & place)
  : RegionWithName(place.GetMultilangName())
  , RegionWithData(place.GetRegionData())
  , m_polygon(std::make_shared<BoostPolygon>())
{
  CHECK_NOT_EQUAL(place.GetPlaceType(), PlaceType::Unknown, ());

  auto const radius = GetRadiusByPlaceType(place.GetPlaceType());
  *m_polygon = MakePolygonWithRadius(place.GetPosition(), radius);
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
}

std::string Region::GetTranslatedOrTransliteratedName(LanguageCode languageCode) const
{
  if (!m_placeLabel)
    return RegionWithName::GetTranslatedOrTransliteratedName(languageCode);

  std::string const & name = m_placeLabel->GetTranslatedOrTransliteratedName(languageCode);
  return name.empty() ? RegionWithName::GetTranslatedOrTransliteratedName(languageCode) : name;
}

std::string Region::GetName(int8_t lang) const
{
  if (m_placeLabel)
    return m_placeLabel->GetName();

  return RegionWithName::GetName();
}

base::GeoObjectId Region::GetId() const
{
  if (m_placeLabel)
    return m_placeLabel->GetId();

  return RegionWithData::GetId();
}

PlaceType Region::GetPlaceType() const
{
  if (m_placeLabel)
    return m_placeLabel->GetPlaceType();

  return RegionWithData::GetPlaceType();
}

boost::optional<std::string> Region::GetIsoCode() const
{
  if (m_placeLabel)
  {
    if (auto isoCode = m_placeLabel->GetIsoCode())
      return isoCode;
  }

  return RegionWithData::GetIsoCode();
}

void Region::SetLabel(PlacePoint const & place)
{
  CHECK(!m_placeLabel, ());
  m_placeLabel = place;
}

// static
double Region::GetRadiusByPlaceType(PlaceType place)
{
  // Based on average radiuses of OSM place polygons.
  switch (place)
  {
  case PlaceType::City: return 0.078;
  case PlaceType::Town: return 0.033;
  case PlaceType::Village: return 0.013;
  case PlaceType::Hamlet: return 0.0067;
  case PlaceType::Suburb: return 0.016;
  case PlaceType::Quarter:
  case PlaceType::Neighbourhood:
  case PlaceType::IsolatedDwelling: return 0.0035;
  case PlaceType::Unknown:
  default: UNREACHABLE();
  }
  UNREACHABLE();
}

void Region::FillPolygon(FeatureBuilder const & fb)
{
  CHECK(m_polygon, ());
  boost_helpers::FillPolygon(*m_polygon, fb);
}

bool Region::IsLocality() const { return GetPlaceType() >= PlaceType::City; }

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
  auto const min =
      std::min(boost::geometry::area(*other.m_polygon), boost::geometry::area(*m_polygon));
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
  if (m_placeLabel)
    return m_placeLabel->GetPosition();

  BoostPoint p;
  boost::geometry::centroid(m_rect, p);
  return p;
}

bool Region::Contains(PlacePoint const & place) const
{
  CHECK(m_polygon, ());

  return Contains(place.GetPosition());
}

bool Region::Contains(BoostPoint const & point) const
{
  CHECK(m_polygon, ());

  return boost::geometry::covered_by(point, m_rect) &&
         boost::geometry::covered_by(point, *m_polygon);
}

std::string GetRegionNotation(Region const & region)
{
  auto notation = region.GetTranslatedOrTransliteratedName(StringUtf8Multilang::GetLangIndex("en"));
  if (notation.empty())
    return region.GetName();

  if (notation != region.GetName())
    notation += " / " + region.GetName();
  return notation;
}
}  // namespace regions
}  // namespace generator

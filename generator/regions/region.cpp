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
Region::Region(FeatureBuilder const & fb, RegionDataProxy const & rd)
  : RegionWithName(fb.GetParams().name)
  , RegionWithData(rd)
  , m_polygon(std::make_shared<BoostPolygon>())
{
  FillPolygon(fb);
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
  CHECK_GREATER_OR_EQUAL(m_area, 0.0, ());
}

Region::Region(StringUtf8Multilang const & name, RegionDataProxy const & rd,
               std::shared_ptr<BoostPolygon> const & polygon)
  : RegionWithName(name), RegionWithData(rd)
{
  SetPolygon(polygon);
}

std::string Region::GetTranslatedOrTransliteratedName(LanguageCode languageCode) const
{
  if (!m_placeLabel)
    return RegionWithName::GetTranslatedOrTransliteratedName(languageCode);

  std::string const & name = m_placeLabel->GetTranslatedOrTransliteratedName(languageCode);
  return name.empty() ? RegionWithName::GetTranslatedOrTransliteratedName(languageCode) : name;
}

std::string Region::GetInternationalName() const
{
  if (!m_placeLabel)
    return RegionWithName::GetInternationalName();

  std::string intName = m_placeLabel->GetInternationalName();

  return intName.empty() ? RegionWithName::GetInternationalName() : intName;
}

std::string Region::GetName(int8_t lang) const
{
  if (m_placeLabel)
    return m_placeLabel->GetName(lang);

  return RegionWithName::GetName(lang);
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

boost::optional<PlacePoint> const & Region::GetLabel() const noexcept { return m_placeLabel; }

void Region::SetLabel(PlacePoint const & place)
{
  CHECK(!m_placeLabel, ());
  m_placeLabel = place;
}

void Region::FillPolygon(FeatureBuilder const & fb)
{
  CHECK(m_polygon, ());
  boost_helpers::FillPolygon(*m_polygon, fb);
}

bool Region::IsLocality() const { return GetPlaceType() >= PlaceType::City; }

void Region::SetPolygon(std::shared_ptr<BoostPolygon> const & polygon)
{
  m_polygon = polygon;
  m_rect = {};
  boost::geometry::envelope(*m_polygon, m_rect);
  m_area = boost::geometry::area(*m_polygon);
  CHECK_GREATER_OR_EQUAL(m_area, 0.0, ());
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
}  // namespace regions
}  // namespace generator

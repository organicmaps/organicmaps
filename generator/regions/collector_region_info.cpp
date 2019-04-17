#include "generator/regions/collector_region_info.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <map>

namespace generator
{
namespace regions
{
std::string const CollectorRegionInfo::kDefaultExt = ".regions.bin";
uint8_t const CollectorRegionInfo::kVersion = 0;

PlaceType EncodePlaceType(std::string const & place)
{
  static std::map<std::string, PlaceType> const m = {
    {"city", PlaceType::City},
    {"town", PlaceType::Town},
    {"village", PlaceType::Village},
    {"suburb", PlaceType::Suburb},
    {"neighbourhood", PlaceType::Neighbourhood},
    {"hamlet", PlaceType::Hamlet},
    {"isolated_dwelling", PlaceType::IsolatedDwelling}
  };

  auto const it = m.find(place);
  return it == m.end() ? PlaceType::Unknown : it->second;
}

char const * GetLabel(PlaceLevel level)
{
  switch (level)
  {
  case PlaceLevel::Country:
    return "country";
  case PlaceLevel::Region:
    return "region";
  case PlaceLevel:: Subregion:
    return "subregion";
  case PlaceLevel::Locality:
    return "locality";
  case PlaceLevel::Suburb:
    return "suburb";
  case PlaceLevel::Sublocality:
    return "sublocality";
  case PlaceLevel::Unknown:
    return nullptr;
  case PlaceLevel::Count:
    UNREACHABLE();
  }
  UNREACHABLE();
}

CollectorRegionInfo::CollectorRegionInfo(std::string const & filename) : m_filename(filename) {}

void CollectorRegionInfo::CollectFeature(const FeatureBuilder1 &, OsmElement const & el)
{
  base::GeoObjectId const osmId = GetGeoObjectId(el);
  RegionData regionData;
  FillRegionData(osmId, el, regionData);
  m_mapRegionData.emplace(osmId, regionData);
  // If the region is a country.
  if (regionData.m_adminLevel == AdminLevel::Two)
  {
    IsoCode isoCode;
    FillIsoCode(osmId, el, isoCode);
    m_mapIsoCode.emplace(osmId, isoCode);
  }
}

void CollectorRegionInfo::Save()
{
  FileWriter writer(m_filename);
  WriteToSink(writer, kVersion);
  WriteMap(writer, m_mapRegionData);
  WriteMap(writer, m_mapIsoCode);
}

void CollectorRegionInfo::FillRegionData(base::GeoObjectId const & osmId, OsmElement const & el,
                                         RegionData & rd)
{
  rd.m_osmId = osmId;
  rd.m_place = EncodePlaceType(el.GetTag("place"));
  auto const al = el.GetTag("admin_level");
  if (al.empty())
    return;

  try
  {
    auto const adminLevel = std::stoi(al);
    // Administrative level is in the range [1 ... 12].
    // https://wiki.openstreetmap.org/wiki/Tag:boundary=administrative
    rd.m_adminLevel = (adminLevel >= 1 && adminLevel <= 12) ?
                        static_cast<AdminLevel>(adminLevel) : AdminLevel::Unknown;
  }
  catch (std::exception const & e)  // std::invalid_argument, std::out_of_range
  {
    LOG(::base::LWARNING, (e.what()));
    rd.m_adminLevel = AdminLevel::Unknown;
  }
}

void CollectorRegionInfo::FillIsoCode(base::GeoObjectId const & osmId, OsmElement const & el,
                                      IsoCode & rd)
{
  rd.m_osmId = osmId;
  rd.SetAlpha2(el.GetTag("ISO3166-1:alpha2"));
  rd.SetAlpha3(el.GetTag("ISO3166-1:alpha3"));
  rd.SetNumeric(el.GetTag("ISO3166-1:numeric"));
}

void IsoCode::SetAlpha2(std::string const & alpha2)
{
  CHECK_LESS_OR_EQUAL(alpha2.size() + 1, ARRAY_SIZE(m_alpha2), ());
  std::strcpy(m_alpha2, alpha2.data());
}

void IsoCode::SetAlpha3(std::string const & alpha3)
{
  CHECK_LESS_OR_EQUAL(alpha3.size() + 1, ARRAY_SIZE(m_alpha3), ());
  std::strcpy(m_alpha3, alpha3.data());
}

void IsoCode::SetNumeric(std::string const & numeric)
{
  CHECK_LESS_OR_EQUAL(numeric.size() + 1, ARRAY_SIZE(m_numeric), ());
  std::strcpy(m_numeric, numeric.data());
}
}  // namespace regions
}  // namespace generator

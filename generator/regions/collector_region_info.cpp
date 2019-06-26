#include "generator/regions/collector_region_info.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <map>

using namespace feature;

namespace generator
{
namespace regions
{
std::string const CollectorRegionInfo::kDefaultExt = ".regions.bin";
uint8_t const CollectorRegionInfo::kVersion = 0;

PlaceType EncodePlaceType(std::string const & place)
{
  static std::map<std::string, PlaceType> const m = {
    {"country", PlaceType::Country},
    {"state", PlaceType::State},
    {"province", PlaceType::Province},
    {"district", PlaceType::District},
    {"county", PlaceType::County},
    {"municipality", PlaceType::Municipality},
    {"city", PlaceType::City},
    {"town", PlaceType::Town},
    {"village", PlaceType::Village},
    {"hamlet", PlaceType::Hamlet},
    {"isolated_dwelling", PlaceType::IsolatedDwelling},
    {"suburb", PlaceType::Suburb},
    {"quarter", PlaceType::Quarter},
    {"neighbourhood", PlaceType::Neighbourhood},
  };

  auto const it = m.find(place);
  return it == m.end() ? PlaceType::Unknown : it->second;
}

char const * StringifyPlaceType(PlaceType placeType)
{
  switch (placeType)
  {
  case PlaceType::Country:
    return "country";
  case PlaceType::State:
    return "state";
  case PlaceType::Province:
    return "province";
  case PlaceType::District:
    return "district";
  case PlaceType::County:
    return "county";
  case PlaceType::Municipality:
    return "municipality";
  case PlaceType::City:
    return "city";
  case PlaceType::Town:
    return "town";
  case PlaceType::Village:
    return "village";
  case PlaceType::Hamlet:
    return "hamlet";
  case PlaceType::IsolatedDwelling:
    return "isolated_dwelling";
  case PlaceType::Suburb:
    return "suburb";
  case PlaceType::Quarter:
    return "quarter";
  case PlaceType::Neighbourhood:
    return "neighbourhood";
  case PlaceType::Unknown:
    return "unknown";
  };

  UNREACHABLE();
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

CollectorRegionInfo::CollectorRegionInfo(std::string const & filename)
  : CollectorInterface(filename) {}


std::shared_ptr<CollectorInterface>
CollectorRegionInfo::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CollectorRegionInfo>(GetFilename());
}

void CollectorRegionInfo::Collect(OsmElement const & el)
{
  base::GeoObjectId const osmId = GetGeoObjectId(el);
  RegionData regionData;
  FillRegionData(osmId, el, regionData);
  m_mapRegionData.emplace(osmId, regionData);
  // If the region is a country.
  if (regionData.m_place == PlaceType::Country || regionData.m_adminLevel == AdminLevel::Two)
  {
    IsoCode isoCode;
    FillIsoCode(osmId, el, isoCode);
    m_mapIsoCode.emplace(osmId, isoCode);
  }
}

void CollectorRegionInfo::Save()
{
  FileWriter writer(GetFilename());
  WriteToSink(writer, kVersion);
  WriteMap(writer, m_mapRegionData);
  WriteMap(writer, m_mapIsoCode);
}

void CollectorRegionInfo::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CollectorRegionInfo::MergeInto(CollectorRegionInfo & collector) const
{
  collector.m_mapRegionData.insert(std::begin(m_mapRegionData), std::end(m_mapRegionData));
  collector.m_mapIsoCode.insert(std::begin(m_mapIsoCode), std::end(m_mapIsoCode));
}

void CollectorRegionInfo::FillRegionData(base::GeoObjectId const & osmId, OsmElement const & el,
                                         RegionData & rd)
{
  rd.m_osmId = osmId;
  rd.m_place = EncodePlaceType(el.GetTag("place"));

  auto const al = el.GetTag("admin_level");
  if (!al.empty())
  {
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
      LOG(LWARNING, (e.what()));
      rd.m_adminLevel = AdminLevel::Unknown;
    }
  }

  for (auto const & member : el.Members())
  {
    if (member.m_role == "label" && member.m_type == OsmElement::EntityType::Node)
      rd.m_labelOsmId = base::MakeOsmNode(member.m_ref);
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

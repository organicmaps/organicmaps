#include "generator/region_info_collector.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <map>

namespace
{
uint8_t const kVersion = 0;
}  // namespace

namespace generator
{
std::string const RegionInfoCollector::kDefaultExt = ".regions.bin";

PlaceType EncodePlaceType(std::string const & place)
{
  static std::map<std::string, PlaceType> const m = {
    {"city", PlaceType::City},
    {"town", PlaceType::Town},
    {"village", PlaceType::Village},
    {"suburb", PlaceType::Suburb},
    {"neighbourhood", PlaceType::Neighbourhood},
    {"hamlet", PlaceType::Hamlet},
    {"locality", PlaceType::Locality},
    {"isolated_dwelling", PlaceType::IsolatedDwelling}
  };

  auto const it = m.find(place);
  return it == m.end() ? PlaceType::Unknown : it->second;
}

void RegionInfoCollector::IsoCode::SetAlpha2(std::string const & alpha2)
{
  CHECK_LESS_OR_EQUAL(alpha2.size() + 1, ARRAY_SIZE(m_alpha2), ());
  std::strcpy(m_alpha2, alpha2.data());
}

void RegionInfoCollector::IsoCode::SetAlpha3(std::string const & alpha3)
{
  CHECK_LESS_OR_EQUAL(alpha3.size() + 1, ARRAY_SIZE(m_alpha3), ());
  std::strcpy(m_alpha3, alpha3.data());
}

void RegionInfoCollector::IsoCode::SetNumeric(std::string const & numeric)
{
  CHECK_LESS_OR_EQUAL(numeric.size() + 1, ARRAY_SIZE(m_numeric), ());
  std::strcpy(m_numeric, numeric.data());
}

RegionInfoCollector::RegionInfoCollector(std::string const & filename)
{
  ParseFile(filename);
}

RegionInfoCollector::RegionInfoCollector(Platform::FilesList const & filenames)
{
  for (auto const & filename : filenames)
    ParseFile(filename);
}

void RegionInfoCollector::ParseFile(std::string const & filename)
{
  try
  {
    FileReader reader(filename);
    ReaderSource<FileReader> src(reader);
    uint8_t version;
    ReadPrimitiveFromSource(src, version);
    CHECK_EQUAL(version, kVersion, ("Versions do not match."));
    ReadMap(src, m_mapRegionData);
    ReadMap(src, m_mapIsoCode);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LCRITICAL, ("Failed to parse regions info:", e.Msg()));
  }
}

void RegionInfoCollector::Add(base::GeoObjectId const & osmId, OsmElement const & el)
{
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

void RegionInfoCollector::Save(std::string const & filename)
{
  try
  {
    FileWriter writer(filename);
    WriteToSink(writer, kVersion);
    WriteMap(writer, m_mapRegionData);
    WriteMap(writer, m_mapIsoCode);
  }
  catch (FileWriter::Exception const & e)
  {
    LOG(LCRITICAL, ("Failed to save regions info:", e.Msg()));
  }
}

RegionDataProxy RegionInfoCollector::Get(base::GeoObjectId const & osmId) const
{
  return RegionDataProxy(*this, osmId);
}

void RegionInfoCollector::FillRegionData(base::GeoObjectId const & osmId, OsmElement const & el,
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
    rd.m_adminLevel = (adminLevel >= 1 || adminLevel <= 12) ?
                        static_cast<AdminLevel>(adminLevel) : AdminLevel::Unknown;
  }
  catch (std::exception const & e)  // std::invalid_argument, std::out_of_range
  {
    LOG(::my::LWARNING, (e.what()));
    rd.m_adminLevel = AdminLevel::Unknown;
  }
}

void RegionInfoCollector::FillIsoCode(base::GeoObjectId const & osmId, OsmElement const & el,
                                      IsoCode & rd)
{
  rd.m_osmId = osmId;
  rd.SetAlpha2(el.GetTag("ISO3166-1:alpha2"));
  rd.SetAlpha3(el.GetTag("ISO3166-1:alpha3"));
  rd.SetNumeric(el.GetTag("ISO3166-1:numeric"));
}

RegionDataProxy::RegionDataProxy(RegionInfoCollector const & regionInfoCollector,
                                 base::GeoObjectId const & osmId)
  : m_regionInfoCollector(regionInfoCollector),
    m_osmId(osmId)
{
}

RegionInfoCollector const & RegionDataProxy::GetCollector() const
{
  return m_regionInfoCollector;
}

RegionInfoCollector::MapRegionData const & RegionDataProxy::GetMapRegionData() const
{
  return GetCollector().m_mapRegionData;
}

RegionInfoCollector::MapIsoCode const & RegionDataProxy::GetMapIsoCode() const
{
  return GetCollector().m_mapIsoCode;
}

base::GeoObjectId const & RegionDataProxy::GetOsmId() const
{
  return m_osmId;
}

AdminLevel RegionDataProxy::GetAdminLevel() const
{
  return GetMapRegionData().at(m_osmId).m_adminLevel;
}

PlaceType RegionDataProxy::GetPlaceType() const
{
  return GetMapRegionData().at(m_osmId).m_place;
}

bool RegionDataProxy::HasAdminLevel() const
{
  return (GetMapRegionData().count(m_osmId) != 0) &&
      (GetMapRegionData().at(m_osmId).m_adminLevel != AdminLevel::Unknown);
}

bool RegionDataProxy::HasPlaceType() const
{
  return (GetMapRegionData().count(m_osmId) != 0) &&
      (GetMapRegionData().at(m_osmId).m_place != PlaceType::Unknown);
}

bool RegionDataProxy::HasIsoCode() const
{
  return GetMapIsoCode().count(m_osmId) != 0;
}

bool RegionDataProxy::HasIsoCodeAlpha2() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasAlpha2();
}

bool RegionDataProxy::HasIsoCodeAlpha3() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasAlpha3();
}

bool RegionDataProxy::HasIsoCodeAlphaNumeric() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasNumeric();
}

std::string RegionDataProxy::GetIsoCodeAlpha2() const
{
  return GetMapIsoCode().at(m_osmId).GetAlpha2();
}

std::string RegionDataProxy::GetIsoCodeAlpha3() const
{
  return GetMapIsoCode().at(m_osmId).GetAlpha3();
}

std::string RegionDataProxy::GetIsoCodeAlphaNumeric() const
{
  return GetMapIsoCode().at(m_osmId).GetNumeric();
}
}  // namespace generator

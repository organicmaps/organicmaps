#include "generator/region_info_collector.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/logging.hpp"

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
    uint32_t size;
    ReadPrimitiveFromSource(src, size);
    RegionData regionData;
    for (uint32_t i = 0; i < size; ++i)
    {
      ReadPrimitiveFromSource(src, regionData);
      m_map.emplace(regionData.m_osmId, regionData);
    }
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LCRITICAL, ("Failed to parse regions info:", e.Msg()));
  }
}

void RegionInfoCollector::Add(OsmElement const & el)
{
  RegionData regionData;
  Fill(el, regionData);
  m_map.emplace(el.id, regionData);
}

void RegionInfoCollector::Save(std::string const & filename)
{
  try
  {
    FileWriter writer(filename);
    WriteToSink(writer, kVersion);
    uint32_t const size = static_cast<uint32_t>(m_map.size());
    WriteToSink(writer, size);
    for (auto const & el : m_map)
      writer.Write(&el.second, sizeof(el.second));
  }
  catch (FileWriter::Exception const & e)
  {
    LOG(LCRITICAL, ("Failed to save regions info:", e.Msg()));
  }
}

RegionData & RegionInfoCollector::Get(uint64_t osmId)
{
  return m_map.at(osmId);
}

RegionData const & RegionInfoCollector::Get(uint64_t osmId) const
{
  return m_map.at(osmId);
}

bool RegionInfoCollector::Exists(uint64_t osmId) const
{
  return m_map.count(osmId) != 0;
}

void RegionInfoCollector::Fill(OsmElement const & el, RegionData & rd)
{
  rd.m_osmId = el.id;
  rd.m_place = EncodePlaceType(el.GetTag("place"));
  auto const al = el.GetTag("admin_level");
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
    LOG(::my::LERROR, (e.what()));
    rd.m_adminLevel = AdminLevel::Unknown;
  }
}
}  // namespace generator

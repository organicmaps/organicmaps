#include "generator/regions/region_info.hpp"

#include "generator/regions/collector_region_info.hpp"

#include "coding/file_reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

namespace generator
{
namespace regions
{
RegionInfo::RegionInfo(std::string const & filename)
{
  ParseFile(filename);
}

RegionInfo::RegionInfo(Platform::FilesList const & filenames)
{
  for (auto const & filename : filenames)
    ParseFile(filename);
}

void RegionInfo::ParseFile(std::string const & filename)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  uint8_t version;
  ReadPrimitiveFromSource(src, version);
  CHECK_EQUAL(version, CollectorRegionInfo::kVersion, ());
  ReadMap(src, m_mapRegionData);
  ReadMap(src, m_mapIsoCode);
}

RegionDataProxy RegionInfo::Get(base::GeoObjectId const & osmId)
{
  return RegionDataProxy(*this, osmId);
}

ConstRegionDataProxy RegionInfo::Get(base::GeoObjectId const & osmId) const
{
  return ConstRegionDataProxy(*this, osmId);
}

RegionInfo & RegionDataProxy::GetCollector()
{
  return m_regionInfoCollector;
}

MapRegionData & RegionDataProxy::GetMapRegionData()
{
  return GetCollector().m_mapRegionData;
}

void RegionDataProxy::SetAdminLevel(AdminLevel adminLevel)
{
  GetMapRegionData().at(m_osmId).m_adminLevel = adminLevel;
}

void RegionDataProxy::SetPlaceType(PlaceType placeType)
{
  GetMapRegionData().at(m_osmId).m_place = placeType;
}

template <typename T>
BaseRegionDataProxy<T>::BaseRegionDataProxy(T & regionInfoCollector, base::GeoObjectId const & osmId)
  : m_regionInfoCollector(regionInfoCollector), m_osmId(osmId) {}

template <typename T>
RegionInfo const & BaseRegionDataProxy<T>::GetCollector() const
{
  return m_regionInfoCollector;
}

template <typename T>
MapRegionData const & BaseRegionDataProxy<T>::GetMapRegionData() const
{
  return GetCollector().m_mapRegionData;
}

template <typename T>
MapIsoCode const & BaseRegionDataProxy<T>::GetMapIsoCode() const
{
  return GetCollector().m_mapIsoCode;
}

template <typename T>
base::GeoObjectId const & BaseRegionDataProxy<T>::GetOsmId() const
{
  return m_osmId;
}

template <typename T>
AdminLevel BaseRegionDataProxy<T>::GetAdminLevel() const
{
  auto const & regionDataMap = GetMapRegionData();
  auto const regionData = regionDataMap.find(m_osmId);
  if (regionData == end(regionDataMap))
    return AdminLevel::Unknown;

  return regionData->second.m_adminLevel;
}

template <typename T>
PlaceType BaseRegionDataProxy<T>::GetPlaceType() const
{
  auto const & regionDataMap = GetMapRegionData();
  auto const regionData = regionDataMap.find(m_osmId);
  if (regionData == end(regionDataMap))
    return PlaceType::Unknown;

  return regionData->second.m_place;
}

template <typename T>
boost::optional<base::GeoObjectId> BaseRegionDataProxy<T>::GetLabelOsmId() const
{
  auto const & labelId = GetMapRegionData().at(m_osmId).m_labelOsmId;
  if (!labelId.GetEncodedId())
    return {};

  return labelId;
}

template <typename T>
boost::optional<std::string> BaseRegionDataProxy<T>::GetIsoCodeAlpha2() const
{
  auto const & codesMap = GetMapIsoCode();
  auto const code = codesMap.find(m_osmId);
  if (code == end(codesMap))
    return {};

  if (!code->second.HasAlpha2())
    return {};
  return code->second.GetAlpha2();
}

template <typename T>
boost::optional<std::string> BaseRegionDataProxy<T>::GetIsoCodeAlpha3() const
{
  auto const & codesMap = GetMapIsoCode();
  auto const code = codesMap.find(m_osmId);
  if (code == end(codesMap))
    return {};

  if (!code->second.HasAlpha3())
    return {};
  return code->second.GetAlpha3();
}

template <typename T>
boost::optional<std::string> BaseRegionDataProxy<T>::GetIsoCodeAlphaNumeric() const
{
  auto const & codesMap = GetMapIsoCode();
  auto const code = codesMap.find(m_osmId);
  if (code == end(codesMap))
    return {};

  if (!code->second.HasNumeric())
    return {};
  return code->second.GetNumeric();
}

template class BaseRegionDataProxy<RegionInfo>;
template class BaseRegionDataProxy<RegionInfo const>;
}  // namespace regions
}  // namespace generator

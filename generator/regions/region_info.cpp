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
  return GetMapRegionData().at(m_osmId).m_adminLevel;
}

template <typename T>
PlaceType BaseRegionDataProxy<T>::GetPlaceType() const
{
  return GetMapRegionData().at(m_osmId).m_place;
}

template <typename T>
bool BaseRegionDataProxy<T>::HasAdminLevel() const
{
  return (GetMapRegionData().count(m_osmId) != 0) &&
      (GetMapRegionData().at(m_osmId).m_adminLevel != AdminLevel::Unknown);
}

template <typename T>
bool BaseRegionDataProxy<T>::HasPlaceType() const
{
  return (GetMapRegionData().count(m_osmId) != 0) &&
      (GetMapRegionData().at(m_osmId).m_place != PlaceType::Unknown);
}

template <typename T>
bool BaseRegionDataProxy<T>::HasIsoCode() const
{
  return GetMapIsoCode().count(m_osmId) != 0;
}

template <typename T>
bool BaseRegionDataProxy<T>::HasIsoCodeAlpha2() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasAlpha2();
}

template <typename T>
bool BaseRegionDataProxy<T>::HasIsoCodeAlpha3() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasAlpha3();
}

template <typename T>
bool BaseRegionDataProxy<T>::HasIsoCodeAlphaNumeric() const
{
  return HasIsoCode() && GetMapIsoCode().at(m_osmId).HasNumeric();
}

template <typename T>
std::string BaseRegionDataProxy<T>::GetIsoCodeAlpha2() const
{
  return GetMapIsoCode().at(m_osmId).GetAlpha2();
}

template <typename T>
std::string BaseRegionDataProxy<T>::GetIsoCodeAlpha3() const
{
  return GetMapIsoCode().at(m_osmId).GetAlpha3();
}

template <typename T>
std::string BaseRegionDataProxy<T>::GetIsoCodeAlphaNumeric() const
{
  return GetMapIsoCode().at(m_osmId).GetNumeric();
}

template class BaseRegionDataProxy<RegionInfo>;
template class BaseRegionDataProxy<RegionInfo const>;
}  // namespace regions
}  // namespace generator

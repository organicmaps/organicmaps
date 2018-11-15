#pragma once

#include "generator/regions/collector_region_info.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace generator
{
namespace regions
{
template<typename T>
class BaseRegionDataProxy;
class RegionDataProxy;
class ConstRegionDataProxy;

// RegionInfo class is responsible for reading and accessing additional information about the regions.
class RegionInfo
{
public:
  RegionInfo() = default;
  explicit RegionInfo(std::string const & filename);
  explicit RegionInfo(Platform::FilesList const & filenames);

  RegionDataProxy Get(base::GeoObjectId const & osmId);
  ConstRegionDataProxy Get(base::GeoObjectId const & osmId) const;

private:
  friend class BaseRegionDataProxy<RegionInfo>;
  friend class BaseRegionDataProxy<RegionInfo const>;
  friend class RegionDataProxy;
  friend class ConstRegionDataProxy;

  template <typename Source, typename Map>
  void ReadMap(Source & src, Map & seq)
  {
    uint32_t size = 0;
    ReadPrimitiveFromSource(src, size);
    typename Map::mapped_type data;
    for (uint32_t i = 0; i < size; ++i)
    {
      ReadPrimitiveFromSource(src, data);
      seq.emplace(data.m_osmId, std::move(data));
    }
  }

  void ParseFile(std::string const & filename);

  MapRegionData m_mapRegionData;
  MapIsoCode m_mapIsoCode;
};

template<typename T>
class BaseRegionDataProxy
{
public:
  BaseRegionDataProxy(T & regionInfoCollector, base::GeoObjectId const & osmId);

  base::GeoObjectId const & GetOsmId() const;
  AdminLevel GetAdminLevel() const;
  PlaceType GetPlaceType() const;

  bool HasAdminLevel() const;
  bool HasPlaceType() const;

  bool HasIsoCodeAlpha2() const;
  bool HasIsoCodeAlpha3() const;
  bool HasIsoCodeAlphaNumeric() const;

  std::string GetIsoCodeAlpha2() const;
  std::string GetIsoCodeAlpha3() const;
  std::string GetIsoCodeAlphaNumeric() const;

protected:
  bool HasIsoCode() const;
  RegionInfo const & GetCollector() const;
  MapRegionData const & GetMapRegionData() const;
  MapIsoCode const & GetMapIsoCode() const;

  std::reference_wrapper<T> m_regionInfoCollector;
  base::GeoObjectId m_osmId;
};

class ConstRegionDataProxy : public BaseRegionDataProxy<RegionInfo const>
{
public:
  using BaseRegionDataProxy<RegionInfo const>::BaseRegionDataProxy;
};

class RegionDataProxy : public BaseRegionDataProxy<RegionInfo>
{
public:
  using BaseRegionDataProxy<RegionInfo>::BaseRegionDataProxy;

  void SetAdminLevel(AdminLevel adminLevel);
  void SetPlaceType(PlaceType placeType);

private:
  RegionInfo & GetCollector();
  MapRegionData & GetMapRegionData();
};
}  // namespace regions
}  // namespace generator

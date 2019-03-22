#pragma once

#include "generator/collector_interface.hpp"

#include "platform/platform.hpp"

#include "coding/write_to_sink.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>

struct OsmElement;
class FeatureParams;
class FileWriter;

namespace generator
{
namespace regions
{
// https://wiki.openstreetmap.org/wiki/Tag:boundary=administrative
enum class AdminLevel : uint8_t
{
  Unknown = 0,
  One = 1,
  Two = 2,
  Three = 3,
  Four = 4,
  Five = 5,
  Six = 6,
  Seven = 7,
  Eight = 8,
  Nine = 9,
  Ten = 10,
  Eleven = 11,
  Twelve = 12,
};

// https://wiki.openstreetmap.org/wiki/Key:place
// Warning: values are important, be careful they are used in Region::GetRank() in regions.cpp
enum class PlaceType: uint8_t
{
  Unknown = 0,
  City = 9,
  Town = 10,
  Village = 11,
  Hamlet = 12,
  Suburb = 13,
  Neighbourhood = 14,
  Locality = 15,
  IsolatedDwelling = 16,
};

PlaceType EncodePlaceType(std::string const & place);

// Codes for the names of countries, dependent territories, and special areas of geographical
// interest.
// https://en.wikipedia.org/wiki/ISO_3166-1
struct IsoCode
{
  bool HasAlpha2() const { return m_alpha2[0] != '\0'; }
  bool HasAlpha3() const { return m_alpha3[0] != '\0'; }
  bool HasNumeric() const { return m_numeric[0] != '\0'; }

  void SetAlpha2(std::string const & alpha2);
  void SetAlpha3(std::string const & alpha3);
  void SetNumeric(std::string const & numeric);

  std::string GetAlpha2() const { return m_alpha2; }
  std::string GetAlpha3() const { return m_alpha3; }
  std::string GetNumeric() const { return m_numeric; }

  base::GeoObjectId m_osmId;
  char m_alpha2[3] = {};
  char m_alpha3[4] = {};
  char m_numeric[4] = {};
};

struct RegionData
{
  base::GeoObjectId m_osmId;
  AdminLevel m_adminLevel = AdminLevel::Unknown;
  PlaceType m_place = PlaceType::Unknown;
};

using MapRegionData = std::unordered_map<base::GeoObjectId, RegionData>;
using MapIsoCode = std::unordered_map<base::GeoObjectId, IsoCode>;

// CollectorRegionInfo class is responsible for collecting additional information about regions.
class CollectorRegionInfo : public CollectorInterface
{
public:
  static uint8_t const kVersion;
  static std::string const kDefaultExt;

  CollectorRegionInfo(std::string const & filename);

  // CollectorInterface overrides:
  void CollectFeature(FeatureBuilder1 const &, OsmElement const & el) override;
  void Save() override;

private:
  template <typename Sink, typename Map>
  void WriteMap(Sink & sink, Map & seq)
  {
    static_assert(std::is_trivially_copyable<typename Map::mapped_type>::value, "");

    uint32_t const sizeRegionData = static_cast<uint32_t>(seq.size());
    WriteToSink(sink, sizeRegionData);
    for (auto const & el : seq)
      sink.Write(&el.second, sizeof(el.second));
  }
  void FillRegionData(base::GeoObjectId const & osmId, OsmElement const & el, RegionData & rd);
  void FillIsoCode(base::GeoObjectId const & osmId, OsmElement const & el, IsoCode & rd);

  std::string m_filename;
  MapRegionData m_mapRegionData;
  MapIsoCode m_mapIsoCode;
};

inline std::ostream & operator<<(std::ostream & out, AdminLevel const & t)
{
  out << static_cast<int>(t);
  return out;
}

inline std::ostream & operator<<(std::ostream & out, PlaceType const & t)
{
  out << static_cast<int>(t);
  return out;
}
}  // namespace regions
}  // namespace generator

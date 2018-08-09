#pragma once

#include "platform/platform.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>

struct OsmElement;
class FeatureParams;
class FileWriter;

namespace generator
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
  Suburb = 12,
  Neighbourhood = 13,
  Hamlet = 14,
  Locality = 15,
  IsolatedDwelling = 16,
};

PlaceType EncodePlaceType(std::string const & place);

struct RegionData
{
  uint64_t m_osmId = 0;
  AdminLevel m_adminLevel = AdminLevel::Unknown;
  PlaceType m_place = PlaceType::Unknown;
};

// This is a class for working a file with additional information about regions.
class RegionInfoCollector
{
public:
  static std::string const kDefaultExt;

  RegionInfoCollector() = default;
  explicit RegionInfoCollector(std::string const & filename);
  explicit RegionInfoCollector(Platform::FilesList const & filenames);

  // It is supposed to be called already on the filtered osm objects that represent regions.
  void Add(OsmElement const & el);
  // osmId is osm relation id.
  RegionData & Get(uint64_t osmId);
  const RegionData & Get(uint64_t osmId) const;
  bool Exists(uint64_t osmId) const;
  void Save(std::string const & filename);

private:
  void ParseFile(std::string const & filename);
  void Fill(OsmElement const & el, RegionData & rd);

  std::unordered_map<uint64_t, RegionData> m_map;
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
}  // namespace generator

#pragma once

#include "topography_generator/isolines_utils.hpp"

#include "storage/storage_defines.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"

#include "base/visitor.hpp"

#include <map>
#include <string>
#include <unordered_set>

namespace topography_generator
{
struct IsolinesProfile
{
  uint32_t m_alitudesStep = 10;
  uint8_t m_latLonStepFactor = 1;
  uint32_t m_maxIsolinesLength = 1000;
  uint8_t m_simplificationZoom = 17;     // Value == 0 disables simplification.
  uint8_t m_medianFilterR = 1;           // Value == 0 disables filter.
  double m_gaussianFilterStDev = 2.0;    // Value == 0.0 disables filter.
  double m_gaussianFilterRFactor = 1.0;  // Value == 0.0 disables filter.

  DECLARE_VISITOR_AND_DEBUG_PRINT(IsolinesProfile, visitor(m_alitudesStep, "alitudesStep"),
                                  visitor(m_latLonStepFactor, "latLonStepFactor"),
                                  visitor(m_maxIsolinesLength, "maxIsolinesLength"),
                                  visitor(m_simplificationZoom, "simplificationZoom"),
                                  visitor(m_medianFilterR, "medianFilterR"),
                                  visitor(m_gaussianFilterStDev, "gaussianFilterStDev"),
                                  visitor(m_gaussianFilterRFactor, "gaussianFilterRFactor"))
};

struct IsolinesProfilesCollection
{
  std::map<std::string, IsolinesProfile> m_profiles;

  DECLARE_VISITOR_AND_DEBUG_PRINT(IsolinesProfilesCollection, visitor(m_profiles, "profiles"))
};

struct TileCoord
{
  TileCoord() = default;
  TileCoord(int bottomLat, int leftLon) : m_leftLon(leftLon), m_bottomLat(bottomLat) {}

  int32_t m_leftLon = 0;
  int32_t m_bottomLat = 0;

  bool operator==(TileCoord const & rhs) const { return m_leftLon == rhs.m_leftLon && m_bottomLat == rhs.m_bottomLat; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(TileCoord, visitor(m_bottomLat, "bottomLat"), visitor(m_leftLon, "leftLon"))
};

struct TileCoordHash
{
  size_t operator()(TileCoord const & coord) const
  {
    return (static_cast<size_t>(coord.m_leftLon) << 32u) | static_cast<size_t>(coord.m_bottomLat);
  }
};

struct CountryIsolinesParams
{
  std::string m_profileName;
  std::unordered_set<TileCoord, TileCoordHash> m_tileCoordsSubset;
  bool m_tilesAreBanned;

  bool NeedSkipTile(int lat, int lon) const
  {
    if (m_tileCoordsSubset.empty())
      return false;
    TileCoord coord(lat, lon);
    auto const found = m_tileCoordsSubset.find(coord) != m_tileCoordsSubset.end();
    return m_tilesAreBanned == found;
  }

  DECLARE_VISITOR_AND_DEBUG_PRINT(CountryIsolinesParams, visitor(m_profileName, "profileName"),
                                  visitor(m_tileCoordsSubset, "tileCoordsSubset"),
                                  visitor(m_tilesAreBanned, "tilesAreBanned"))
};

struct CountriesToGenerate
{
  std::map<storage::CountryId, CountryIsolinesParams> m_countryParams;

  DECLARE_VISITOR_AND_DEBUG_PRINT(CountriesToGenerate, visitor(m_countryParams, "countryParams"))
};

template <typename DataType>
bool Serialize(std::string const & fileName, DataType const & data)
{
  try
  {
    FileWriter writer(fileName);
    coding::SerializerJson<FileWriter> ser(writer);
    ser(data);
    return true;
  }
  catch (base::Json::Exception & ex)
  {
    LOG(LERROR, ("Serialization to json failed, file name", fileName, ", reason:", ex.Msg()));
  }
  catch (FileWriter::Exception const & ex)
  {
    LOG(LERROR, ("Can't write file", fileName, ", reason:", ex.Msg()));
  }
  return false;
}

template <typename DataType>
bool Deserialize(std::string const & fileName, DataType & data)
{
  try
  {
    FileReader reader(fileName);
    NonOwningReaderSource source(reader);
    coding::DeserializerJson des(source);
    des(data);
    return true;
  }
  catch (base::Json::Exception & ex)
  {
    LOG(LERROR, ("Deserialization from json failed, file name", fileName, ", reason:", ex.Msg()));
  }
  catch (FileReader::Exception const & ex)
  {
    LOG(LERROR, ("Can't read file", fileName, ", reason:", ex.Msg()));
  }
  return false;
}
}  // namespace topography_generator

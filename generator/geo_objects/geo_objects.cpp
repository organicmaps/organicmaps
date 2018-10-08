#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <fstream>
#include <future>
#include <map>

#include "platform/platform.hpp"

#include <boost/optional.hpp>
#include "3party/jansson/myjansson.hpp"

namespace
{
using KeyValue = std::map<uint64_t, base::Json>;
using IndexReader = ReaderPtr<Reader>;

bool ParseKeyValueLine(std::string const & line, std::pair<uint64_t, base::Json> & res)
{
  auto const pos = line.find(" ");
  if (pos == std::string::npos)
  {
    LOG(LWARNING, ("Cannot find separator."));
    return false;
  }

  int64_t id;
  if (!strings::to_int64(line.substr(0, pos), id))
  {
    LOG(LWARNING, ("Cannot parse id."));
    return false;
  }

  base::Json json;
  try
  {
    json = base::Json(line.substr(pos + 1));
    if (!json.get())
      return false;
  }
  catch (base::Json::Exception const &)
  {
    LOG(LWARNING, ("Cannot create base::Json."));
    return false;
  }

  res = std::make_pair(static_cast<uint64_t>(id), json);
  return true;
}

// This is an interface if reading key-value.
class KeyValueInterface
{
public:
  virtual ~KeyValueInterface() = default;

  virtual boost::optional<base::Json> Find(uint64_t key) const = 0;
};

// This is an implementation of reading key-values with loading and searching in memory.
class KeyValueMem : public KeyValueInterface
{
public:
  KeyValueMem(std::istream & stream)
  {
    std::string line;
    std::pair<uint64_t, base::Json> kv;
    while (std::getline(stream, line))
    {
      if (!ParseKeyValueLine(line, kv))
        continue;

      m_map.insert(kv);
    }

  }

  boost::optional<base::Json> Find(uint64_t key) const override
  {
    boost::optional<base::Json> result;
    auto const it = m_map.find(key);
    if (it != std::end(m_map))
      result = it->second;

    return result;
  }

private:
  std::map<uint64_t, base::Json> m_map;
};

// This is an implementation of reading key-values with loading and searching in disk.
class KeyValueMap : public KeyValueInterface
{
public:
  KeyValueMap(std::istream & stream) : m_stream(stream)
  {
    std::string line;
    uint64_t pos = 0;
    std::pair<uint64_t, base::Json> kv;
    while (std::getline(m_stream, line))
    {
      if (!ParseKeyValueLine(line, kv))
        continue;

      m_map.emplace(kv.first, pos);
      pos = m_stream.tellg();
    }
  }

  boost::optional<base::Json> Find(uint64_t key) const override
  {
    boost::optional<base::Json> result;
    auto const it = m_map.find(key);
    if (it == std::end(m_map))
      return result;

    m_stream.seekg(it->second);
    std::string line;
    std::getline(m_stream, line);
    std::pair<uint64_t, base::Json> kv;
    if (!ParseKeyValueLine(line, kv))
      return result;

    result = kv.second;
    return result;
  }

private:
  std::istream & m_stream;
  std::map<uint64_t, uint64_t> m_map;
};

template <typename Index>
std::vector<base::GeoObjectId> SearchObjectsInIndex(FeatureBuilder1 const & fb, Index const & index)
{
  std::vector<base::GeoObjectId> ids;
  auto const fn = [&ids] (base::GeoObjectId const & osmid) { ids.emplace_back(osmid); };
  auto const center = fb.GetLimitRect().Center();
  auto const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, 0 /* meters */);
  index.ForEachInRect(fn, rect);
  return ids;
}

int GetRankFromValue(base::Json const & json)
{
  int rank;
  auto properties = json_object_get(json.get(), "properties");
  FromJSONObject(properties, "rank", rank);
  return rank;
}

base::Json GetDeepestRegion(std::vector<base::GeoObjectId> const & ids,
                            KeyValueInterface const & regionKv)
{
  base::Json deepest;
  int deepestRank = 0;
  for (auto const & id : ids)
  {
    base::Json temp;
    auto const res = regionKv.Find(id.GetEncodedId());
    if (!res)
    {
      LOG(LWARNING, ("Id not found in region key-value:", id));
      continue;
    }

    temp = *res;
    if (!json_is_object(temp.get()))
    {
      LOG(LWARNING, ("Value is not a json object in region key-value:", id));
      continue;
    }

    if (!deepest.get())
    {
      deepestRank = GetRankFromValue(temp);
      deepest = temp;
    }
    else
    {
      int tempRank = GetRankFromValue(temp);
      if (deepestRank < tempRank)
      {
        deepest = temp;
        deepestRank = tempRank;
      }
    }
  }

  return deepest;
}

void UpdateCoordinates(FeatureBuilder1 const & fb, base::Json json)
{
  auto geometry = json_object_get(json.get(), "geometry");
  auto coordinates = json_object_get(geometry, "coordinates");
  if (json_array_size(coordinates) == 2)
  {
    auto const & center = fb.GetLimitRect().Center();
    json_array_set_new(coordinates, 0, ToJSON(center.x).release());
    json_array_set_new(coordinates, 1, ToJSON(center.x).release());
  }
}

base::Json AddAddress(FeatureBuilder1 const & fb, base::Json const regionJson)
{
  base::Json result = regionJson.GetDeepCopy();
  int const kHouseRank = 30;
  ToJSONObject(*result.get(), "rank", kHouseRank);
  UpdateCoordinates(fb, result);
  auto properties = json_object_get(result.get(), "properties");
  auto address = json_object_get(properties, "address");
  ToJSONObject(*address, "house", fb.GetParams().house.Get());
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  std::string name = fb.GetParams().GetStreet();
  if (!name.empty())
    name += ", ";

  name += fb.GetParams().house.Get();
  ToJSONObject(*properties, "name", name);
  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithAdrress(FeatureBuilder1 const & fb,
                              indexer::RegionsIndex<IndexReader> const & regionIndex,
                              KeyValueInterface const & regionKv)
{
  auto const ids = SearchObjectsInIndex(fb, regionIndex);
  auto const json = GetDeepestRegion(ids, regionKv);
  auto const jsonWithAddress = AddAddress(fb, json);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  std::unique_ptr<char, JSONFreeDeleter> buffer(cstr);
  return buffer;
}

boost::optional<base::Json>
FindPoiHouse(FeatureBuilder1 const & fb,
             indexer::GeoObjectsIndex<IndexReader> const & geoObjectsIndex,
             KeyValueInterface const & geoObjectsKv)
{
  auto const ids = SearchObjectsInIndex(fb, geoObjectsIndex);
  boost::optional<base::Json> result;
  for (auto const & id : ids)
  {
    auto const value = geoObjectsKv.Find(id.GetEncodedId());
    if (!value)
      continue;

    result = *value;
    auto properties = json_object_get(result.get(), "properties");
    auto address = json_object_get(properties, "address");
    std::string const kHouseField = "house";
    char const * key = nullptr;
    json_t * val = nullptr;
    json_object_foreach(address, key, val)
    {
      if (key == kHouseField)
        return result;
    }
  }

  return result;
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithoutAdrress(FeatureBuilder1 const & fb, base::Json const & json)
{
  auto const jsonWithAddress = json.GetDeepCopy();
  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  ToJSONObject(*properties, "name", fb.GetName());
  UpdateCoordinates(fb, jsonWithAddress);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  std::unique_ptr<char, JSONFreeDeleter> buffer(cstr);
  return buffer;
}

boost::optional<indexer::GeoObjectsIndex<IndexReader>>
MakeTempGeoObjectsIndex(std::string const & pathInGeoObjectsTmpMwm)
{
  boost::optional<indexer::GeoObjectsIndex<IndexReader>> result;
  auto const dataFile = Platform().TmpPathForFile("geo_objects_dat.tmp");
  auto const indexFile = Platform().TmpPathForFile("geo_objects_index.tmp");
  if (!feature::GenerateGeoObjectsData(pathInGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects data."));
    return result;
  }

  if (!indexer::BuildGeoObjectsIndexFromDataFile(dataFile, indexFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects index."));
    return result;
  }

  result = indexer::ReadIndex<indexer::GeoObjectsIndexBox<IndexReader>, MmapReader>(indexFile);
  return result;
}

bool BuildGeoObjectsWithAddresses(indexer::RegionsIndex<IndexReader> const & regionIndex,
                                  KeyValueInterface const & regionKv,
                                  std::string const & pathInGeoObjectsTmpMwm,
                                  std::ostream & streamGeoObjectsKv, bool verbose)
{
  auto const fn =
      [&regionIndex, &regionKv, &streamGeoObjectsKv](FeatureBuilder1 & fb, uint64_t /* currPos */)
  {
    if (fb.GetParams().house.IsEmpty())
      return;

    const auto value = MakeGeoObjectValueWithAdrress(fb, regionIndex, regionKv);
    streamGeoObjectsKv << static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId()) << " "
                       << value.get() << "\n";
  };

  try
  {
    feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }

  return true;
}

bool BuildGeoObjectsWithoutAddresses(indexer::GeoObjectsIndex<IndexReader> const & geoObjectsIndex,
                                     std::string const & pathInGeoObjectsTmpMwm,
                                     KeyValueInterface const & geoObjectsKv,
                                     std::ostream & streamGeoObjectsKv,
                                     std::ostream & streamIdsWithoutAddress, bool verbose)
{
  auto const fn  = [&geoObjectsIndex, &streamIdsWithoutAddress, &streamGeoObjectsKv, &geoObjectsKv]
                   (FeatureBuilder1 & fb, uint64_t /* currPos */)
  {
    if (!fb.GetParams().house.IsEmpty())
      return;

    const auto hasInToHouse = FindPoiHouse(fb, geoObjectsIndex, geoObjectsKv);
    if (!hasInToHouse)
      return;

    auto const value = MakeGeoObjectValueWithoutAdrress(fb, *hasInToHouse);
    auto const id = static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId());
    streamGeoObjectsKv << id << " " << value.get() << "\n";
    streamIdsWithoutAddress << id << "\n";
  };

  try
  {
    feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace

namespace generator
{
namespace geo_objects
{
bool GenerateGeoObjects(std::string const & pathInRegionsIndx,
                        std::string const & pathInRegionsKv,
                        std::string const & pathInGeoObjectsTmpMwm,
                        std::string const & pathOutIdsWithoutAddress,
                        std::string const & pathOutGeoObjectsKv, bool verbose)
{
  // This function generates a key-value for geo objects.
  // The implementation may seem confusing, below are a few words about it.
  // First, we try to generate a key-value only for houses, since we cannot say anything about poi.
  // In this step, we need the key-values for the regions and the index for the regions.
  // Then we build an index for houses. And then we finish building the key-value for poi using
  // this index for houses.
  auto geoObjectIndexFuture = std::async(std::launch::async, MakeTempGeoObjectsIndex,
                                         pathInGeoObjectsTmpMwm);
  auto const regionIndex =
      indexer::ReadIndex<indexer::RegionsIndexBox<IndexReader>, MmapReader>(pathInRegionsIndx);
  // Regions key-value file is small(~150 Mb). We will load everything into memory.
  std::fstream streamRegionKv(pathInRegionsKv);
  auto const regionsKv = KeyValueMem(streamRegionKv);
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  std::ofstream streamGeoObjectsKv(pathOutGeoObjectsKv);
  if (!BuildGeoObjectsWithAddresses(regionIndex, regionsKv, pathInGeoObjectsTmpMwm,
                                    streamGeoObjectsKv, verbose))
  {
    return false;
  }

  // Regions key-value file is big(~80 Gb). We will not load the key value into memory.
  // This can be slow.
  // todo(maksimandrianov1): Investigate the issue of performance and if necessary improve.
  std::ifstream tempStream(pathOutGeoObjectsKv);
  auto const geoObjectsKv = KeyValueMap(tempStream);
  auto const geoObjectIndex = geoObjectIndexFuture.get();
  if (!geoObjectIndex||
      !BuildGeoObjectsWithoutAddresses(*geoObjectIndex, pathInGeoObjectsTmpMwm, geoObjectsKv,
                                       streamGeoObjectsKv, streamIdsWithoutAddress, verbose))
  {
    return false;
  }

  return true;
}
}  // namespace geo_objects
}  // namespace generator

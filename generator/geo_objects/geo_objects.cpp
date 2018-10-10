#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <unordered_map>
#include <utility>

#include "platform/platform.hpp"

#include <boost/optional.hpp>
#include "3party/jansson/myjansson.hpp"

namespace
{
using KeyValue = std::pair<uint64_t, base::Json>;
using IndexReader = ReaderPtr<Reader>;

bool ParseKey(std::string const & line, int64_t & key)
{
  auto const pos = line.find(" ");
  if (pos == std::string::npos)
  {
    LOG(LWARNING, ("Cannot find separator."));
    return false;
  }

  if (!strings::to_int64(line.substr(0, pos), key))
  {
    LOG(LWARNING, ("Cannot parse id."));
    return false;
  }

  return true;
}

bool ParseKeyValueLine(std::string const & line, KeyValue & res)
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

// An interface for reading key-value storage.
class KeyValueInterface
{
public:
  virtual ~KeyValueInterface() = default;

  virtual boost::optional<base::Json> Find(uint64_t key) const = 0;
  virtual size_t Size() const = 0;
};

// An implementation for reading key-value storage with loading and searching in memory.
class KeyValueMem : public KeyValueInterface
{
public:
  KeyValueMem(std::istream & stream)
  {
    std::string line;
    KeyValue kv;
    while (std::getline(stream, line))
    {
      if (!ParseKeyValueLine(line, kv))
        continue;

      m_map.insert(kv);
    }

  }

  // KeyValueInterface overrides:
  boost::optional<base::Json> Find(uint64_t key) const override
  {
    boost::optional<base::Json> result;
    auto const it = m_map.find(key);
    if (it != std::end(m_map))
      result = it->second;

    return result;
  }

  size_t Size() const override { return m_map.size(); }

private:
  std::unordered_map<uint64_t, base::Json> m_map;
};

// An implementation for reading key-value storage with loading and searching in disk.
class KeyValueMap : public KeyValueInterface
{
public:
  KeyValueMap(std::istream & stream) : m_stream(stream)
  {
    std::string line;
    std::istream::pos_type pos = 0;
    KeyValue kv;
    while (std::getline(m_stream, line))
    {
      int64_t key;
      if (!ParseKey(line, key))
        continue;

      m_map.emplace(key, pos);
      pos = m_stream.tellg();
    }

    m_stream.clear();
  }

  // KeyValueInterface overrides:
  boost::optional<base::Json> Find(uint64_t key) const override
  {
    boost::optional<base::Json> result;
    auto const it = m_map.find(key);
    if (it == std::end(m_map))
      return result;

    m_stream.seekg(it->second);
    std::string line;
    if (!std::getline(m_stream, line))
    {
      LOG(LERROR, ("Cannot read line."));
      return result;
    }

    KeyValue kv;
    if (ParseKeyValueLine(line, kv))
      result = kv.second;

    return result;
  }

  size_t Size() const override { return m_map.size(); }

private:
  std::istream & m_stream;
  std::unordered_map<uint64_t, std::istream::pos_type> m_map;
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

int GetRankFromValue(base::Json json)
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
      LOG(LWARNING, ("Id not found in region key-value storage:", id));
      continue;
    }

    temp = *res;
    if (!json_is_object(temp.get()))
    {
      LOG(LWARNING, ("Value is not a json object in region key-value storage:", id));
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

void UpdateCoordinates(m2::PointD const & point, base::Json json)
{
  auto geometry = json_object_get(json.get(), "geometry");
  auto coordinates = json_object_get(geometry, "coordinates");
  if (json_array_size(coordinates) == 2)
  {
    json_array_set_new(coordinates, 0, ToJSON(point.x).release());
    json_array_set_new(coordinates, 1, ToJSON(point.y).release());
  }
}

base::Json AddAddress(FeatureBuilder1 const & fb, base::Json regionJson)
{
  base::Json result = regionJson.GetDeepCopy();
  int const kHouseRank = 30;
  ToJSONObject(*result.get(), "rank", kHouseRank);
  UpdateCoordinates(fb.GetLimitRect().Center(), result);
  auto properties = json_object_get(result.get(), "properties");
  auto address = json_object_get(properties, "address");
  ToJSONObject(*address, "house", fb.GetParams().house.Get());
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

boost::optional<base::Json>
FindRegion(FeatureBuilder1 const & fb, indexer::RegionsIndex<IndexReader> const & regionIndex,
           KeyValueInterface const & regionKv)
{
  boost::optional<base::Json> result;
  auto const ids = SearchObjectsInIndex(fb, regionIndex);
  auto const deepest = GetDeepestRegion(ids, regionKv);
  if (deepest.get())
    result = deepest;

  return result;
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithAddress(FeatureBuilder1 const & fb, base::Json json)
{
  auto const jsonWithAddress = AddAddress(fb, json);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>(cstr);
}

boost::optional<base::Json>
FindHousePoi(FeatureBuilder1 const & fb,
             indexer::GeoObjectsIndex<IndexReader> const & geoObjectsIndex,
             KeyValueInterface const & geoObjectsKv)
{
  auto const ids = SearchObjectsInIndex(fb, geoObjectsIndex);
  for (auto const & id : ids)
  {
    auto const house = geoObjectsKv.Find(id.GetEncodedId());
    if (!house)
      continue;

    auto properties = json_object_get(house->get(), "properties");
    auto address = json_object_get(properties, "address");
    std::string const kHouseField = "house";
    char const * key = nullptr;
    json_t * value = nullptr;
    json_object_foreach(address, key, value)
    {
      if (key == kHouseField)
        return house;
    }
  }

  return {};
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithoutAddress(FeatureBuilder1 const & fb, base::Json json)
{
  auto const jsonWithAddress = json.GetDeepCopy();
  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  ToJSONObject(*properties, "name", fb.GetName());
  UpdateCoordinates(fb.GetLimitRect().Center(), jsonWithAddress);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>(cstr);
}

boost::optional<indexer::GeoObjectsIndex<IndexReader>>
MakeTempGeoObjectsIndex(std::string const & pathToGeoObjectsTmpMwm)
{
  boost::optional<indexer::GeoObjectsIndex<IndexReader>> result;
  auto const dataFile = Platform().TmpPathForFile();
  SCOPE_GUARD(removeDataFile, std::bind(Platform::RemoveFileIfExists, std::cref(dataFile)));
  if (!feature::GenerateGeoObjectsData(pathToGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects data."));
    return result;
  }

  auto const indexFile = Platform().TmpPathForFile();
  SCOPE_GUARD(removeIndexFile, std::bind(Platform::RemoveFileIfExists, std::cref(indexFile)));
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
                                  std::ostream & streamGeoObjectsKv, bool)
{
  size_t countGeoObjects = 0;
  auto const fn = [&](FeatureBuilder1 & fb, uint64_t /* currPos */)
  {
    if (fb.GetParams().house.IsEmpty())
      return;

    auto region = FindRegion(fb, regionIndex, regionKv);
    if (!region)
      return;

    auto const value = MakeGeoObjectValueWithAddress(fb, *region);
    streamGeoObjectsKv << static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId()) << " "
                       << value.get() << "\n";
    ++countGeoObjects;
  };

  try
  {
    feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
    LOG(LINFO, ("Added ", countGeoObjects, "geo objects with addresses."));
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
                                     std::ostream & streamIdsWithoutAddress, bool)
{
  size_t countGeoObjects = 0;
  auto const fn  = [&](FeatureBuilder1 & fb, uint64_t /* currPos */)
  {
    if (!fb.GetParams().house.IsEmpty())
      return;

    auto const house = FindHousePoi(fb, geoObjectsIndex, geoObjectsKv);
    if (!house)
      return;

    auto const value = MakeGeoObjectValueWithoutAddress(fb, *house);
    auto const id = static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId());
    streamGeoObjectsKv << id << " " << value.get() << "\n";
    streamIdsWithoutAddress << id << "\n";
    ++countGeoObjects;
  };

  try
  {
    feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
    LOG(LINFO, ("Added ", countGeoObjects, "geo objects without addresses."));
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
  LOG(LINFO, ("Start generating geo objects.."));
  auto timer = base::Timer();
  SCOPE_GUARD(finishGeneratingGeoObjects, [&timer]() {
    LOG(LINFO, ("Finish generating geo objects.", timer.ElapsedSeconds(), "seconds."));
  });

  auto geoObjectIndexFuture = std::async(std::launch::async, MakeTempGeoObjectsIndex,
                                         pathInGeoObjectsTmpMwm);
  auto const regionIndex =
      indexer::ReadIndex<indexer::RegionsIndexBox<IndexReader>, MmapReader>(pathInRegionsIndx);
  // Regions key-value storage is small (~150 Mb). We will load everything into memory.
  std::fstream streamRegionKv(pathInRegionsKv);
  auto const regionsKv = KeyValueMem(streamRegionKv);
  LOG(LINFO, ("Size of regions key-value storage:", regionsKv.Size()));
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  std::ofstream streamGeoObjectsKv(pathOutGeoObjectsKv);
  if (!BuildGeoObjectsWithAddresses(regionIndex, regionsKv, pathInGeoObjectsTmpMwm,
                                    streamGeoObjectsKv, verbose))
  {
    return false;
  }

  LOG(LINFO, ("Geo objects with addresses were built."));
  // Regions key-value storage is big (~80 Gb). We will not load the key value into memory.
  // This can be slow.
  // todo(maksimandrianov1): Investigate the issue of performance and if necessary improve.
  std::ifstream tempStream(pathOutGeoObjectsKv);
  auto const geoObjectsKv = KeyValueMem(tempStream);
  LOG(LINFO, ("Size of geo objects key-value storage:", geoObjectsKv.Size()));
  auto const geoObjectIndex = geoObjectIndexFuture.get();
  LOG(LINFO, ("Index was built."));
  if (!geoObjectIndex ||
      !BuildGeoObjectsWithoutAddresses(*geoObjectIndex, pathInGeoObjectsTmpMwm, geoObjectsKv,
                                       streamGeoObjectsKv, streamIdsWithoutAddress, verbose))
  {
    return false;
  }

  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to",  pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", pathOutIdsWithoutAddress));
  return true;
}
}  // namespace geo_objects
}  // namespace generator

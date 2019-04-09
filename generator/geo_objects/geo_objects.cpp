#include "generator/geo_objects/geo_objects.hpp"

#include "generator/geo_objects/key_value_storage.hpp"

#include "generator/feature_builder.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <future>

#include "platform/platform.hpp"

#include <boost/optional.hpp>
#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
namespace
{
using IndexReader = ReaderPtr<Reader>;

bool IsBuilding(FeatureBuilder1 const & fb)
{
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  return checker(fb.GetTypes());
}

bool HasHouse(FeatureBuilder1 const & fb)
{
  return !fb.GetParams().house.IsEmpty();
}

bool HouseHasAddress(base::Json json)
{
  auto properties = json_object_get(json.get(), "properties");
  auto address = json_object_get(properties, "address");
  std::string const kHouseField = "building";
  char const * key = nullptr;
  json_t * value = nullptr;
  json_object_foreach(address, key, value)
  {
    if (key == kHouseField && !json_is_null(value))
      return true;
  }

  return false;
}

template <typename Index>
std::vector<base::GeoObjectId> SearchObjectsInIndex(FeatureBuilder1 const & fb, Index const & index)
{
  std::vector<base::GeoObjectId> ids;
  auto const fn = [&ids] (base::GeoObjectId const & osmid) { ids.emplace_back(osmid); };
  auto const center = fb.GetKeyPoint();
  index.ForEachInRect(fn, m2::RectD(center, center));
  return ids;
}

int GetRankFromValue(base::Json json)
{
  int rank;
  auto properties = json_object_get(json.get(), "properties");
  FromJSONObject(properties, "rank", rank);
  return rank;
}

boost::optional<KeyValue> GetDeepestRegion(std::vector<base::GeoObjectId> const & ids,
                                           KeyValueStorage const & regionKv)
{
  boost::optional<KeyValue> deepest;
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

    if (!deepest)
    {
      deepestRank = GetRankFromValue(temp);
      deepest = KeyValue(static_cast<int64_t>(id.GetEncodedId()), temp);
    }
    else
    {
      int tempRank = GetRankFromValue(temp);
      if (deepestRank < tempRank)
      {
        deepest = KeyValue(static_cast<int64_t>(id.GetEncodedId()), temp);
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
    auto const latLon = MercatorBounds::ToLatLon(point);
    json_array_set_new(coordinates, 0, ToJSON(latLon.lat).release());
    json_array_set_new(coordinates, 1, ToJSON(latLon.lon).release());
  }
}

base::Json AddAddress(FeatureBuilder1 const & fb, KeyValue const & regionKeyValue)
{
  base::Json result = regionKeyValue.second.GetDeepCopy();
  int const kHouseOrPoiRank = 30;
  UpdateCoordinates(fb.GetKeyPoint(), result);
  auto properties = json_object_get(result.get(), "properties");
  auto address = json_object_get(properties, "address");
  ToJSONObject(*properties, "rank", kHouseOrPoiRank);
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // By writing home null in the field we can understand that the house has no address.
  auto const house = fb.GetParams().house.Get();
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  else
    ToJSONObject(*address, "building", base::NewJSONNull());

  ToJSONObject(*properties, "pid", regionKeyValue.first);
  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

boost::optional<KeyValue>
FindRegion(FeatureBuilder1 const & fb, indexer::RegionsIndex<IndexReader> const & regionIndex,
           KeyValueStorage const & regionKv)
{
  auto const ids = SearchObjectsInIndex(fb, regionIndex);
  return GetDeepestRegion(ids, regionKv);
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithAddress(FeatureBuilder1 const & fb, KeyValue const & keyValue)
{
  auto const jsonWithAddress = AddAddress(fb, keyValue);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>(cstr);
}

boost::optional<base::Json>
FindHousePoi(FeatureBuilder1 const & fb,
             indexer::GeoObjectsIndex<IndexReader> const & geoObjectsIndex,
             KeyValueStorage const & geoObjectsKv)
{
  auto const ids = SearchObjectsInIndex(fb, geoObjectsIndex);
  for (auto const & id : ids)
  {
    auto const house = geoObjectsKv.Find(id.GetEncodedId());
    if (!house)
      continue;

    auto properties = json_object_get(house->get(), "properties");
    auto address = json_object_get(properties, "address");
    if (json_object_get(address, "building"))
      return house;
  }

  return {};
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithoutAddress(FeatureBuilder1 const & fb, base::Json json)
{
  auto const jsonWithAddress = json.GetDeepCopy();
  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  ToJSONObject(*properties, "name", fb.GetName());
  UpdateCoordinates(fb.GetKeyPoint(), jsonWithAddress);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>(cstr);
}

boost::optional<indexer::GeoObjectsIndex<IndexReader>>
MakeTempGeoObjectsIndex(std::string const & pathToGeoObjectsTmpMwm)
{
  auto const dataFile = Platform().TmpPathForFile();
  SCOPE_GUARD(removeDataFile, std::bind(Platform::RemoveFileIfExists, std::cref(dataFile)));
  if (!feature::GenerateGeoObjectsData(pathToGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects data."));
    return {};
  }

  auto const indexFile = Platform().TmpPathForFile();
  SCOPE_GUARD(removeIndexFile, std::bind(Platform::RemoveFileIfExists, std::cref(indexFile)));
  if (!indexer::BuildGeoObjectsIndexFromDataFile(dataFile, indexFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects index."));
    return {};
  }

  return indexer::ReadIndex<indexer::GeoObjectsIndexBox<IndexReader>, MmapReader>(indexFile);
}

void BuildGeoObjectsWithAddresses(indexer::RegionsIndex<IndexReader> const & regionIndex,
                                  KeyValueStorage const & regionKv,
                                  std::string const & pathInGeoObjectsTmpMwm,
                                  std::ostream & streamGeoObjectsKv, bool)
{
  size_t countGeoObjects = 0;
  auto const fn = [&](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (!(IsBuilding(fb) || HasHouse(fb)))
      return;

    auto regionKeyValue = FindRegion(fb, regionIndex, regionKv);
    if (!regionKeyValue)
      return;

    auto const value = MakeGeoObjectValueWithAddress(fb, *regionKeyValue);
    streamGeoObjectsKv << static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId()) << " "
                       << value.get() << "\n";
    ++countGeoObjects;
  };

  feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
  LOG(LINFO, ("Added ", countGeoObjects, "geo objects with addresses."));
}

void BuildGeoObjectsWithoutAddresses(indexer::GeoObjectsIndex<IndexReader> const & geoObjectsIndex,
                                     std::string const & pathInGeoObjectsTmpMwm,
                                     KeyValueStorage const & geoObjectsKv,
                                     std::ostream & streamGeoObjectsKv,
                                     std::ostream & streamIdsWithoutAddress, bool)
{
  size_t countGeoObjects = 0;
  auto const fn  = [&](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (IsBuilding(fb) || HasHouse(fb))
      return;

    auto const house = FindHousePoi(fb, geoObjectsIndex, geoObjectsKv);
    if (!house)
      return;

    if (!HouseHasAddress(*house))
      return;

    auto const value = MakeGeoObjectValueWithoutAddress(fb, *house);
    auto const id = static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId());
    streamGeoObjectsKv << id << " " << value.get() << "\n";
    streamIdsWithoutAddress << id << "\n";
    ++countGeoObjects;
  };

  feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, fn);
  LOG(LINFO, ("Added ", countGeoObjects, "geo objects without addresses."));
}
}  // namespace

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
  KeyValueStorage const regionsKv(streamRegionKv);
  LOG(LINFO, ("Size of regions key-value storage:", regionsKv.Size()));
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  std::ofstream streamGeoObjectsKv(pathOutGeoObjectsKv);
  BuildGeoObjectsWithAddresses(regionIndex, regionsKv, pathInGeoObjectsTmpMwm,
                               streamGeoObjectsKv, verbose);
  LOG(LINFO, ("Geo objects with addresses were built."));
  std::ifstream tempStream(pathOutGeoObjectsKv);
  auto const pred = [](KeyValue const & kv) { return HouseHasAddress(kv.second); };
  KeyValueStorage const geoObjectsKv(tempStream, pred);
  LOG(LINFO, ("Size of geo objects key-value storage:", geoObjectsKv.Size()));
  auto const geoObjectIndex = geoObjectIndexFuture.get();
  LOG(LINFO, ("Index was built."));
  if (!geoObjectIndex)
    return false;

  BuildGeoObjectsWithoutAddresses(*geoObjectIndex, pathInGeoObjectsTmpMwm, geoObjectsKv,
                                  streamGeoObjectsKv, streamIdsWithoutAddress, verbose);
  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to",  pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", pathOutIdsWithoutAddress));
  return true;
}
}  // namespace geo_objects
}  // namespace generator

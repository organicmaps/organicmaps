#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/geo_objects/geo_object_info_getter.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"
#include "generator/key_value_storage.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"
#include "generator/regions/region_info_getter.hpp"

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
#include <mutex>

#include "platform/platform.hpp"

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

using namespace feature;

namespace generator
{
namespace geo_objects
{
namespace
{
using IndexReader = ReaderPtr<Reader>;

bool HouseHasAddress(JsonValue const & json)
{
  auto && address =
      base::GetJSONObligatoryFieldByPath(json, "properties", "locales", "default", "address");

  auto && building = base::GetJSONOptionalField(address, "building");
  return building && !base::JSONIsNull(building);
}

void UpdateCoordinates(m2::PointD const & point, base::JSONPtr & json)
{
  auto geometry = json_object_get(json.get(), "geometry");
  auto coordinates = json_object_get(geometry, "coordinates");
  if (json_array_size(coordinates) == 2)
  {
    auto const latLon = MercatorBounds::ToLatLon(point);
    json_array_set_new(coordinates, 0, ToJSON(latLon.m_lon).release());
    json_array_set_new(coordinates, 1, ToJSON(latLon.m_lat).release());
  }
}

base::JSONPtr AddAddress(FeatureBuilder const & fb, KeyValue const & regionKeyValue)
{
  auto result = regionKeyValue.second->MakeDeepCopyJson();
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

  ToJSONObject(*properties, "dref", KeyValueStorage::SerializeDref(regionKeyValue.first));
  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

std::shared_ptr<JsonValue> FindHousePoi(FeatureBuilder const & fb,
                                        GeoObjectInfoGetter const & geoObjectInfoGetter)
{
  return geoObjectInfoGetter.Find(fb.GetKeyPoint(), HouseHasAddress);
}

base::JSONPtr MakeGeoObjectValueWithoutAddress(FeatureBuilder const & fb, JsonValue const & json)
{
  auto jsonWithAddress = json.MakeDeepCopyJson();
  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  ToJSONObject(*properties, "name", fb.GetName());
  UpdateCoordinates(fb.GetKeyPoint(), jsonWithAddress);
  return jsonWithAddress;
}

boost::optional<indexer::GeoObjectsIndex<IndexReader>> MakeTempGeoObjectsIndex(
    std::string const & pathToGeoObjectsTmpMwm)
{
  auto const dataFile = Platform().TmpPathForFile();
  SCOPE_GUARD(removeDataFile, std::bind(Platform::RemoveFileIfExists, std::cref(dataFile)));
  if (!GenerateGeoObjectsData(pathToGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
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

void FilterAddresslessByCountryAndRepackMwm(std::string const & pathInGeoObjectsTmpMwm,
                                            std::string const & includeCountries,
                                            regions::RegionInfoGetter const & regionInfoGetter,
                                            size_t threadsCount)
{
  auto const path = Platform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::mutex collectorMutex;
  auto concurrentCollect = [&](FeatureBuilder const & fb) {
    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  auto const filteringCollector = [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
    if (GeoObjectsFilter::HasHouse(fb))
    {
      concurrentCollect(fb);
      return;
    }

    static_assert(
        std::is_base_of<regions::ConcurrentGetProcessability, regions::RegionInfoGetter>::value,
        "");
    auto regionKeyValue = regionInfoGetter.FindDeepest(fb.GetKeyPoint());
    if (!regionKeyValue)
      return;

    auto && country = base::GetJSONObligatoryFieldByPath(
        *regionKeyValue->second, "properties", "locales", "default", "address", "country");
    auto countryName = FromJSON<std::string>(country);
    auto pos = includeCountries.find(countryName);
    if (pos != std::string::npos)
      concurrentCollect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, filteringCollector);

  Platform().RemoveFileIfExists(pathInGeoObjectsTmpMwm);
  if (std::rename(path.c_str(), pathInGeoObjectsTmpMwm.c_str()) != 0)
    LOG(LERROR, ("Error: Cannot rename", path, "to", pathInGeoObjectsTmpMwm));
}

void BuildGeoObjectsWithAddresses(KeyValueStorage & geoObjectsKv,
                                  regions::RegionInfoGetter const & regionInfoGetter,
                                  std::string const & pathInGeoObjectsTmpMwm, bool verbose,
                                  size_t threadsCount)
{
  std::mutex updateMutex;
  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsBuilding(fb) && !GeoObjectsFilter::HasHouse(fb))
      return;

    auto regionKeyValue = regionInfoGetter.FindDeepest(fb.GetKeyPoint());
    if (!regionKeyValue)
      return;

    auto const id = fb.GetMostGenericOsmId().GetEncodedId();
    auto jsonValue = AddAddress(fb, *regionKeyValue);

    std::lock_guard<std::mutex> lock(updateMutex);
    geoObjectsKv.Insert(id, JsonValue{std::move(jsonValue)});
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO, ("Added", geoObjectsKv.Size(), "geo objects with addresses."));
}

void BuildGeoObjectsWithoutAddresses(KeyValueStorage & geoObjectsKv,
                                     GeoObjectInfoGetter const & geoObjectInfoGetter,
                                     std::string const & pathInGeoObjectsTmpMwm,
                                     std::ostream & streamIdsWithoutAddress, bool verbose,
                                     size_t threadsCount)
{
  auto addressObjectsCount = geoObjectsKv.Size();

  std::mutex updateMutex;
  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsPoi(fb))
      return;
    if (GeoObjectsFilter::IsBuilding(fb) || GeoObjectsFilter::HasHouse(fb))
      return;

    auto const house = FindHousePoi(fb, geoObjectInfoGetter);
    if (!house)
      return;

    auto const id = fb.GetMostGenericOsmId().GetEncodedId();
    auto jsonValue = MakeGeoObjectValueWithoutAddress(fb, *house);

    std::lock_guard<std::mutex> lock(updateMutex);
    geoObjectsKv.Insert(id, JsonValue{std::move(jsonValue)});
    streamIdsWithoutAddress << id << "\n";
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO,
      ("Added ", geoObjectsKv.Size() - addressObjectsCount, "geo objects without addresses."));
}
}  // namespace

bool GenerateGeoObjects(std::string const & pathInRegionsIndex, std::string const & pathInRegionsKv,
                        std::string const & pathInGeoObjectsTmpMwm,
                        std::string const & pathOutIdsWithoutAddress,
                        std::string const & pathOutGeoObjectsKv,
                        std::string const & allowAddresslessForCountries, bool verbose,
                        size_t threadsCount)
{
  LOG(LINFO, ("Start generating geo objects.."));
  auto timer = base::Timer();
  SCOPE_GUARD(finishGeneratingGeoObjects, [&timer]() {
    LOG(LINFO, ("Finish generating geo objects.", timer.ElapsedSeconds(), "seconds."));
  });

  regions::RegionInfoGetter regionInfoGetter{pathInRegionsIndex, pathInRegionsKv};
  LOG(LINFO, ("Size of regions key-value storage:", regionInfoGetter.GetStorage().Size()));

  if (allowAddresslessForCountries != "*")
  {
    FilterAddresslessByCountryAndRepackMwm(pathInGeoObjectsTmpMwm, allowAddresslessForCountries,
                                           regionInfoGetter, threadsCount);
    LOG(LINFO,
        ("Addressless buildings are filtered except countries", allowAddresslessForCountries, "."));
  }

  auto geoObjectIndexFuture =
      std::async(std::launch::async, MakeTempGeoObjectsIndex, pathInGeoObjectsTmpMwm);

  Platform().RemoveFileIfExists(pathOutGeoObjectsKv);
  KeyValueStorage geoObjectsKv(pathOutGeoObjectsKv, 0 /* cacheValuesCountLimit */);
  BuildGeoObjectsWithAddresses(geoObjectsKv, regionInfoGetter, pathInGeoObjectsTmpMwm, verbose,
                               threadsCount);
  LOG(LINFO, ("Geo objects with addresses were built."));

  auto geoObjectIndex = geoObjectIndexFuture.get();
  LOG(LINFO, ("Index was built."));
  if (!geoObjectIndex)
    return false;

  GeoObjectInfoGetter geoObjectInfoGetter{std::move(*geoObjectIndex), geoObjectsKv};
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  BuildGeoObjectsWithoutAddresses(geoObjectsKv, geoObjectInfoGetter, pathInGeoObjectsTmpMwm,
                                  streamIdsWithoutAddress, verbose, threadsCount);
  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to", pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", pathOutIdsWithoutAddress));
  return true;
}
}  // namespace geo_objects
}  // namespace generator

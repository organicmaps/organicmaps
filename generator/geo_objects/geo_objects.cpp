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

bool HouseHasAddress(base::Json const & json)
{
  auto && properties = base::GetJSONObligatoryField(json.get(), "properties");
  auto && address = base::GetJSONObligatoryField(properties, "address");
  auto && building = base::GetJSONOptionalField(address, "building");
  return building && !base::JSONIsNull(building);
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

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValueWithAddress(FeatureBuilder1 const & fb, KeyValue const & keyValue)
{
  auto const jsonWithAddress = AddAddress(fb, keyValue);
  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>(cstr);
}

boost::optional<base::Json>
FindHousePoi(FeatureBuilder1 const & fb, GeoObjectInfoGetter const & geoObjectInfoGetter)
{
  return geoObjectInfoGetter.Find(fb.GetKeyPoint(), HouseHasAddress);
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

void FilterAddresslessByCountryAndRepackMwm(std::string const & pathInGeoObjectsTmpMwm,
                                            std::string_view const & includeCountries,
                                            regions::RegionInfoGetter const & regionInfoGetter)
{
  auto const path = Platform().TmpPathForFile();
  feature::FeaturesCollector collector(path);

  auto const filteringCollector = [&](FeatureBuilder1 const & fb, uint64_t /* currPos */)
  {
    if (GeoObjectsFilter::HasHouse(fb))
    {
      collector(fb);
      return;
    }

    auto regionKeyValue = regionInfoGetter.FindDeepest(fb.GetKeyPoint());
    if (!regionKeyValue)
      return;

    auto && properties = base::GetJSONObligatoryField(regionKeyValue->second.get(), "properties");
    auto && address = base::GetJSONObligatoryField(properties, "address");
    auto && country = base::GetJSONObligatoryField(address, "country");
    auto countryName = FromJSON<std::string_view>(country);
    auto pos = includeCountries.find(countryName);
    if (pos != std::string_view::npos)
      collector(fb);
  };
  feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, filteringCollector);

  Platform().RemoveFileIfExists(pathInGeoObjectsTmpMwm);
  if (std::rename(path.c_str(), pathInGeoObjectsTmpMwm.c_str()) != 0)
    LOG(LERROR, ("Error: Cannot rename", path, "to", pathInGeoObjectsTmpMwm));
}

void BuildGeoObjectsWithAddresses(regions::RegionInfoGetter const & regionInfoGetter,
                                  std::string const & pathInGeoObjectsTmpMwm,
                                  std::ostream & streamGeoObjectsKv, bool)
{
  size_t countGeoObjects = 0;
  auto const fn = [&](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsBuilding(fb) && !GeoObjectsFilter::HasHouse(fb))
      return;

    auto regionKeyValue = regionInfoGetter.FindDeepest(fb.GetKeyPoint());
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

void BuildGeoObjectsWithoutAddresses(GeoObjectInfoGetter const & geoObjectInfoGetter,
                                     std::string const & pathInGeoObjectsTmpMwm,
                                     std::ostream & streamGeoObjectsKv,
                                     std::ostream & streamIdsWithoutAddress, bool)
{
  size_t countGeoObjects = 0;
  auto const fn  = [&](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsPoi(fb))
      return;
    if (GeoObjectsFilter::IsBuilding(fb) || GeoObjectsFilter::HasHouse(fb))
      return;

    auto const house = FindHousePoi(fb, geoObjectInfoGetter);
    if (!house)
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

bool GenerateGeoObjects(std::string const & pathInRegionsIndex,
                        std::string const & pathInRegionsKv,
                        std::string const & pathInGeoObjectsTmpMwm,
                        std::string const & pathOutIdsWithoutAddress,
                        std::string const & pathOutGeoObjectsKv,
                        std::string const & allowAddresslessForCountries, bool verbose)
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
                                           regionInfoGetter);
    LOG(LINFO, ("Addressless buildings are filtered except countries",
                allowAddresslessForCountries, "."));
  }

  auto geoObjectIndexFuture = std::async(std::launch::async, MakeTempGeoObjectsIndex,
                                         pathInGeoObjectsTmpMwm);

  std::ofstream streamGeoObjectsKv(pathOutGeoObjectsKv);

  BuildGeoObjectsWithAddresses(regionInfoGetter, pathInGeoObjectsTmpMwm, streamGeoObjectsKv, verbose);
  LOG(LINFO, ("Geo objects with addresses were built."));

  auto const pred = [](KeyValue const & kv) { return HouseHasAddress(kv.second); };
  KeyValueStorage geoObjectsKv(pathOutGeoObjectsKv, pred);
  LOG(LINFO, ("Size of geo objects key-value storage:", geoObjectsKv.Size()));

  auto geoObjectIndex = geoObjectIndexFuture.get();
  LOG(LINFO, ("Index was built."));
  if (!geoObjectIndex)
    return false;

  GeoObjectInfoGetter geoObjectInfoGetter{std::move(*geoObjectIndex), std::move(geoObjectsKv)};
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  BuildGeoObjectsWithoutAddresses(geoObjectInfoGetter, pathInGeoObjectsTmpMwm,
                                  streamGeoObjectsKv, streamIdsWithoutAddress, verbose);
  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to",  pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", pathOutIdsWithoutAddress));
  return true;
}
}  // namespace geo_objects
}  // namespace generator

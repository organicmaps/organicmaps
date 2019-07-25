#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/geo_objects/geo_object_info_getter.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"
#include "generator/key_value_storage.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <mutex>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

using namespace feature;

namespace generator
{
namespace geo_objects
{
namespace
{
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

  UpdateCoordinates(fb.GetKeyPoint(), result);

  auto properties = base::GetJSONObligatoryField(result.get(), "properties");
  auto address = base::GetJSONObligatoryFieldByPath(properties, "locales", "default", "address");
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // By writing home null in the field we can understand that the house has no address.
  auto const house = fb.GetParams().house.Get();
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  else
    ToJSONObject(*address, "building", base::NewJSONNull());

  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(fb.GetMultilangName()));

  int const kHouseOrPoiRank = 30;
  ToJSONObject(*properties, "rank", kHouseOrPoiRank);

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
  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(fb.GetMultilangName()));

  UpdateCoordinates(fb.GetKeyPoint(), jsonWithAddress);
  return jsonWithAddress;
}

void FilterAddresslessByCountryAndRepackMwm(
    std::string const & pathInGeoObjectsTmpMwm, std::string const & includeCountries,
    GeoObjectsGenerator::RegionInfoGetterProxy const & regionInfoGetter, size_t threadsCount)
{
  auto const path = GetPlatform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::mutex collectorMutex;
  auto concurrentCollect = [&](FeatureBuilder const & fb) {
    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  auto const filteringCollector = [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
    if (GeoObjectsFilter::HasHouse(fb) || GeoObjectsFilter::IsPoi(fb))
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
  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
}

void AddThingsWithHousesAndBuildingsAndEnrichWithRegionAddresses(
    KeyValueStorage & geoObjectsKv,
    GeoObjectsGenerator::RegionInfoGetterProxy const & regionInfoGetter,
    std::string const & pathInGeoObjectsTmpMwm, bool verbose, size_t threadsCount)
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

void AddPoisEnrichedWithHouseAddresses(KeyValueStorage & geoObjectsKv,
                                       GeoObjectInfoGetter const & geoObjectInfoGetter,
                                       std::string const & pathInGeoObjectsTmpMwm,
                                       std::ostream & streamIdsWithoutAddress, bool verbose,
                                       size_t threadsCount)
{
  auto const addressObjectsCount = geoObjectsKv.Size();

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

struct NullBuildingsInfo
{
  std::unordered_map<base::GeoObjectId, base::GeoObjectId> m_addressPoints2Buildings;
  std::set<base::GeoObjectId> m_buildingsIds;
};

NullBuildingsInfo GetHelpfulNullBuildings(GeoObjectInfoGetter const & geoObjectInfoGetter,
                                          std::string const & pathInGeoObjectsTmpMwm,
                                          size_t threadsCount)
{
  NullBuildingsInfo result;

  std::mutex updateMutex;
  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::HasHouse(fb) || !fb.IsPoint())
      return;

    auto const buildingId = geoObjectInfoGetter.Search(
        fb.GetKeyPoint(), [](JsonValue const & json) { return !HouseHasAddress(json); });
    if (!buildingId)
      return;

    auto const id = fb.GetMostGenericOsmId();

    std::lock_guard<std::mutex> lock(updateMutex);
    result.m_addressPoints2Buildings[id] = *buildingId;
    result.m_buildingsIds.insert(*buildingId);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, saveIdFold);
  return result;
}

using BuildingsGeometries =
    std::unordered_map<base::GeoObjectId, feature::FeatureBuilder::Geometry>;

BuildingsGeometries GetBuildingsGeometry(std::string const & pathInGeoObjectsTmpMwm,
                                         NullBuildingsInfo const & buildingsInfo,
                                         size_t threadsCount)
{
  BuildingsGeometries result;
  std::mutex updateMutex;

  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    if (buildingsInfo.m_buildingsIds.find(id) == buildingsInfo.m_buildingsIds.end() ||
        fb.GetParams().GetGeomType() != feature::GeomType::Area)
      return;

    std::lock_guard<std::mutex> lock(updateMutex);

    if (result.find(id) != result.end())
      LOG(LINFO, ("More than one geometry for", id));
    else
      result[id] = fb.GetGeometry();
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, saveIdFold);
  return result;
}

size_t AddBuildingGeometriesToAddressPoints(std::string const & pathInGeoObjectsTmpMwm,
                                            NullBuildingsInfo const & buildingsInfo,
                                            BuildingsGeometries const & geometries,
                                            size_t threadsCount)
{
  auto const path = GetPlatform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::atomic_size_t pointsEnriched{0};
  std::mutex collectorMutex;

  auto concurrentCollector = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    auto point2BuildingIt = buildingsInfo.m_addressPoints2Buildings.find(id);
    if (point2BuildingIt != buildingsInfo.m_addressPoints2Buildings.end())
    {
      auto const & geometry = geometries.at(point2BuildingIt->second);

      // ResetGeometry does not reset center but SetCenter changes geometry type to Point and
      // adds center to bounding rect
      fb.SetCenter({});
      // ResetGeometry clears bounding rect
      fb.ResetGeometry();
      fb.GetParams().SetGeomType(feature::GeomType::Area);

      for (std::vector<m2::PointD> poly : geometry)
        fb.AddPolygon(poly);

      fb.PreSerialize();
      ++pointsEnriched;
    }
    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentCollector);

  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
  return pointsEnriched;
}

void EnrichPointsWithOuterBuildingGeometry(GeoObjectInfoGetter const & geoObjectInfoGetter,
                                           std::string const & pathInGeoObjectsTmpMwm,
                                           size_t threadsCount)
{
  auto const buildingInfo =
      GetHelpfulNullBuildings(geoObjectInfoGetter, pathInGeoObjectsTmpMwm, threadsCount);

  LOG(LINFO, ("Found", buildingInfo.m_addressPoints2Buildings.size(),
              "address points with outer building geometry"));
  LOG(LINFO, ("Found", buildingInfo.m_buildingsIds.size(), "helpful addressless buildings"));
  auto const buildingGeometries =
      GetBuildingsGeometry(pathInGeoObjectsTmpMwm, buildingInfo, threadsCount);
  LOG(LINFO, ("Saved", buildingGeometries.size(), "buildings geometries"));

  size_t const pointsCount = AddBuildingGeometriesToAddressPoints(
      pathInGeoObjectsTmpMwm, buildingInfo, buildingGeometries, threadsCount);

  LOG(LINFO, (pointsCount, "address points were enriched with outer building geomery"));
}

template <class Activist>
auto Measure(std::string activity, Activist && activist)
{
  LOG(LINFO, ("Start", activity));
  auto timer = base::Timer();
  SCOPE_GUARD(_, [&]() { LOG(LINFO, ("Finish", activity, timer.ElapsedSeconds(), "seconds.")); });

  return activist();
}
}  // namespace

boost::optional<indexer::GeoObjectsIndex<IndexReader>> MakeTempGeoObjectsIndex(
    std::string const & pathToGeoObjectsTmpMwm)
{
  auto const dataFile = GetPlatform().TmpPathForFile();
  SCOPE_GUARD(removeDataFile, std::bind(Platform::RemoveFileIfExists, std::cref(dataFile)));
  if (!GenerateGeoObjectsData(pathToGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects data."));
    return {};
  }

  auto const indexFile = GetPlatform().TmpPathForFile();
  SCOPE_GUARD(removeIndexFile, std::bind(Platform::RemoveFileIfExists, std::cref(indexFile)));
  if (!indexer::BuildGeoObjectsIndexFromDataFile(dataFile, indexFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects index."));
    return {};
  }

  return indexer::ReadIndex<indexer::GeoObjectsIndexBox<IndexReader>, MmapReader>(indexFile);
}

GeoObjectsGenerator::GeoObjectsGenerator(
    std::string pathInRegionsIndex, std::string pathInRegionsKv, std::string pathInGeoObjectsTmpMwm,
    std::string pathOutIdsWithoutAddress, std::string pathOutGeoObjectsKv,
    std::string allowAddresslessForCountries, bool verbose, size_t threadsCount)

  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutIdsWithoutAddress(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_allowAddresslessForCountries(std::move(allowAddresslessForCountries))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(pathInRegionsIndex, pathInRegionsKv)

{
}

GeoObjectsGenerator::GeoObjectsGenerator(RegionInfoGetter && regionInfoGetter,
                                         std::string pathInGeoObjectsTmpMwm,
                                         std::string pathOutIdsWithoutAddress,
                                         std::string pathOutGeoObjectsKv,
                                         std::string allowAddresslessForCountries, bool verbose,
                                         size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutIdsWithoutAddress(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_allowAddresslessForCountries(std::move(allowAddresslessForCountries))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(std::move(regionInfoGetter))
{
}

bool GeoObjectsGenerator::GenerateGeoObjects()
{
  return Measure("generating geo objects", [&]() { return GenerateGeoObjectsPrivate(); });
}

bool GeoObjectsGenerator::GenerateGeoObjectsPrivate()
{
  auto geoObjectIndexFuture =
      std::async(std::launch::async, MakeTempGeoObjectsIndex, m_pathInGeoObjectsTmpMwm);

  AddThingsWithHousesAndBuildingsAndEnrichWithRegionAddresses(
      m_geoObjectsKv, m_regionInfoGetter, m_pathInGeoObjectsTmpMwm, m_verbose, m_threadsCount);

  LOG(LINFO, ("Geo objects with addresses were built."));

  auto geoObjectIndex = geoObjectIndexFuture.get();

  LOG(LINFO, ("Index was built."));
  if (!geoObjectIndex)
    return false;

  GeoObjectInfoGetter const geoObjectInfoGetter{std::move(*geoObjectIndex), m_geoObjectsKv};

  LOG(LINFO, ("Enrich address points with outer null building geometry."));

  EnrichPointsWithOuterBuildingGeometry(geoObjectInfoGetter, m_pathInGeoObjectsTmpMwm,
                                        m_threadsCount);

  std::ofstream streamIdsWithoutAddress(m_pathOutIdsWithoutAddress);

  AddPoisEnrichedWithHouseAddresses(m_geoObjectsKv, geoObjectInfoGetter, m_pathInGeoObjectsTmpMwm,
                                    streamIdsWithoutAddress, m_verbose, m_threadsCount);

  if (m_allowAddresslessForCountries != "*")
  {
    FilterAddresslessByCountryAndRepackMwm(m_pathInGeoObjectsTmpMwm, m_allowAddresslessForCountries,
                                           m_regionInfoGetter, m_threadsCount);

    LOG(LINFO, ("Addressless buildings are filtered except countries",
                m_allowAddresslessForCountries, "."));
  }

  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to", m_pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", m_pathOutIdsWithoutAddress));
  return true;
}
}  // namespace geo_objects
}  // namespace generator

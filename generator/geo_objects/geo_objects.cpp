#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/key_value_storage.hpp"
#include "generator/locality_sorter.hpp"

#include "generator/geo_objects/geo_objects.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"
#include "generator/geo_objects/geo_objects_maintainer.hpp"

#include "generator/regions/region_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <mutex>

using namespace feature;

namespace generator
{
namespace geo_objects
{
namespace
{
NullBuildingsInfo GetHelpfulNullBuildings(GeoObjectMaintainer & geoObjectMaintainer,
                                          std::string const & pathInGeoObjectsTmpMwm,
                                          size_t threadsCount)
{
  NullBuildingsInfo result;
  static int64_t counter = 0;
  std::mutex updateMutex;
  auto const & view = geoObjectMaintainer.CreateView();

  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::HasHouse(fb) || !fb.IsPoint())
      return;

    // search for ids of Buildinds not stored with geoObjectsMantainer
    // they are nullBuildings
    auto const buildingId =
        view.SearchIdOfFirstMatchedObject(fb.GetKeyPoint(), [&view](base::GeoObjectId id) {
          auto const & geoData = view.GetGeoData(id);
          return geoData && geoData->m_house.empty();
        });

    if (!buildingId)
      return;

    auto const id = fb.GetMostGenericOsmId();

    std::lock_guard<std::mutex> lock(updateMutex);
    result.m_addressPoints2Buildings[id] = *buildingId;
    counter++;
    if (counter % 100000 == 0)
      LOG(LINFO, (counter, "Helpful building added"));
    result.m_Buildings2AddressPoint[*buildingId] = id;
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
  static int64_t counter = 0;

  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    if (buildingsInfo.m_Buildings2AddressPoint.find(id) ==
            buildingsInfo.m_Buildings2AddressPoint.end() ||
        fb.GetParams().GetGeomType() != feature::GeomType::Area)
      return;

    std::lock_guard<std::mutex> lock(updateMutex);

    if (result.find(id) != result.end())
      LOG(LINFO, ("More than one geometry for", id));
    else
      result[id] = fb.GetGeometry();

    counter++;
    if (counter % 100000 == 0)
      LOG(LINFO, (counter, "Building geometries added"));
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
      auto geometryIt = geometries.find(point2BuildingIt->second);
      if (geometryIt != geometries.end())
      {
        auto const & geometry = geometryIt->second;

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
        if (pointsEnriched % 100000 == 0)
          LOG(LINFO, (pointsEnriched, "Points enriched with geometry"));
      }
      else
      {
        LOG(LINFO, (point2BuildingIt->second, "is a null building with strange geometry"));
      }
    }
    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentCollector);

  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
  return pointsEnriched;
}

base::JSONPtr FindHouse(FeatureBuilder const & fb,
                        GeoObjectMaintainer::GeoObjectsView const & geoView,
                        NullBuildingsInfo const & buildingsInfo)
{

  base::JSONPtr house =
      geoView.GetFullGeoObject(fb.GetKeyPoint(), [](GeoObjectMaintainer::GeoObjectData const & data) {
        return !data.m_house.empty();
      });

  if (house)
    return house;

  std::vector<base::GeoObjectId> potentialIds = geoView.SearchObjectsInIndex(fb.GetKeyPoint());

  for (base::GeoObjectId id : potentialIds)
  {
    auto const it = buildingsInfo.m_Buildings2AddressPoint.find(id);
    if (it != buildingsInfo.m_Buildings2AddressPoint.end())
      return geoView.GetFullGeoObjectWithoutNameAndCoordinates(it->second);
  }

  return {};
}

base::JSONPtr MakeJsonValueWithNameFromFeature(FeatureBuilder const & fb, JsonValue const & json)
{
  auto jsonWithAddress = json.MakeDeepCopyJson();

  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(fb.GetMultilangName()));

  UpdateCoordinates(fb.GetKeyPoint(), jsonWithAddress);
  return jsonWithAddress;
}
}  // namespace

bool JsonHasBuilding(JsonValue const & json)
{
  auto && address =
      base::GetJSONObligatoryFieldByPath(json, "properties", "locales", "default", "address");

  auto && building = base::GetJSONOptionalField(address, "building");
  return building && !base::JSONIsNull(building);
}

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

void AddBuildingsAndThingsWithHousesThenEnrichAllWithRegionAddresses(
    GeoObjectMaintainer & geoObjectMaintainer, std::string const & pathInGeoObjectsTmpMwm,
    bool verbose, size_t threadsCount)
{
  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    geoObjectMaintainer.StoreAndEnrich(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO, ("Added", geoObjectMaintainer.Size(), "geo objects with addresses."));
}

NullBuildingsInfo EnrichPointsWithOuterBuildingGeometry(GeoObjectMaintainer & geoObjectMaintainer,
                                                        std::string const & pathInGeoObjectsTmpMwm,
                                                        size_t threadsCount)
{
  auto const buildingInfo =
      GetHelpfulNullBuildings(geoObjectMaintainer, pathInGeoObjectsTmpMwm, threadsCount);

  LOG(LINFO, ("Found", buildingInfo.m_addressPoints2Buildings.size(),
              "address points with outer building geometry"));
  LOG(LINFO,
      ("Found", buildingInfo.m_Buildings2AddressPoint.size(), "helpful addressless buildings"));
  auto const buildingGeometries =
      GetBuildingsGeometry(pathInGeoObjectsTmpMwm, buildingInfo, threadsCount);
  LOG(LINFO, ("Saved", buildingGeometries.size(), "buildings geometries"));

  size_t const pointsCount = AddBuildingGeometriesToAddressPoints(
      pathInGeoObjectsTmpMwm, buildingInfo, buildingGeometries, threadsCount);

  LOG(LINFO, (pointsCount, "address points were enriched with outer building geomery"));
  return buildingInfo;
}

void AddPoisEnrichedWithHouseAddresses(GeoObjectMaintainer & geoObjectMaintainer,
                                       NullBuildingsInfo const & buildingsInfo,
                                       std::string const & pathInGeoObjectsTmpMwm,
                                       std::ostream & streamPoiIdsToAddToLocalityIndex,
                                       bool verbose, size_t threadsCount)
{
  std::atomic_size_t counter{0};
  std::mutex streamMutex;
  auto const & view = geoObjectMaintainer.CreateView();

  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsPoi(fb))
      return;
    if (GeoObjectsFilter::IsBuilding(fb) || GeoObjectsFilter::HasHouse(fb))
      return;

    // No name and coordinates here, we will take it from fb in MakeJsonValueWithNameFromFeature
    auto house = FindHouse(fb, view, buildingsInfo);
    if (!house)
      return;

    auto const id = fb.GetMostGenericOsmId();
    auto jsonValue = MakeJsonValueWithNameFromFeature(fb, JsonValue{std::move(house)});

    counter++;
    if (counter % 100000 == 0)
      LOG(LINFO, (counter, "pois added"));

    geoObjectMaintainer.WriteToStorage(id, JsonValue{std::move(jsonValue)});

    std::lock_guard<std::mutex> lock(streamMutex);
    streamPoiIdsToAddToLocalityIndex << id << "\n";
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO, ("Added", counter, "POIs enriched with address."));
}

void FilterAddresslessThanGaveTheirGeometryToInnerPoints(std::string const & pathInGeoObjectsTmpMwm,
                                                         NullBuildingsInfo const & buildingsInfo,
                                                         size_t threadsCount)
{
  auto const path = GetPlatform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::mutex collectorMutex;
  auto concurrentCollect = [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    if (buildingsInfo.m_Buildings2AddressPoint.find(id) !=
        buildingsInfo.m_Buildings2AddressPoint.end())
      return;

    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentCollect);
  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
}
}  // namespace geo_objects
}  // namespace generator

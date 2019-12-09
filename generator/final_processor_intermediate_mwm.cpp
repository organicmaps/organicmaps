#include "generator/final_processor_intermediate_mwm.hpp"

#include "generator/affiliation.hpp"
#include "generator/booking_dataset.hpp"
#include "generator/feature_merger.hpp"
#include "generator/mini_roundabout_transformer.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm2type.hpp"
#include "generator/place_processor.hpp"
#include "generator/promo_catalog_cities.hpp"
#include "generator/routing_city_boundaries_processor.hpp"
#include "generator/type_helper.hpp"
#include "generator/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iterator>
#include <memory>
#include <tuple>
#include <vector>

#include "defines.hpp"

using namespace base::thread_pool::computational;
using namespace feature;
using namespace serialization_policy;

namespace generator
{
namespace
{
template <typename ToDo>
void ForEachCountry(std::string const & temporaryMwmPath, ToDo && toDo)
{
  Platform::FilesList fileList;
  Platform::GetFilesByExt(temporaryMwmPath, DATA_FILE_EXTENSION_TMP, fileList);
  for (auto const & filename : fileList)
    toDo(filename);
}

std::vector<std::vector<std::string>> GetAffiliations(std::vector<FeatureBuilder> const & fbs,
                                                      AffiliationInterface const & affiliation,
                                                      size_t threadsCount)
{
  ThreadPool pool(threadsCount);
  std::vector<std::future<std::vector<std::string>>> futuresAffiliations;
  for (auto const & fb : fbs)
  {
    auto result = pool.Submit([&]() { return affiliation.GetAffiliations(fb); });
    futuresAffiliations.emplace_back(std::move(result));
  }

  std::vector<std::vector<std::string>> resultAffiliations;
  resultAffiliations.reserve(futuresAffiliations.size());
  for (auto & f : futuresAffiliations)
    resultAffiliations.emplace_back(f.get());

  return resultAffiliations;
}

// Writes |fbs| to countries tmp.mwm files that |fbs| belongs to according to |affiliations|.
template <class SerializationPolicy = MaxAccuracy>
void AppendToCountries(std::vector<FeatureBuilder> const & fbs,
                       std::vector<std::vector<std::string>> const & affiliations,
                       std::string const & temporaryMwmPath, size_t threadsCount)
{
  std::unordered_map<std::string, std::vector<size_t>> countryToFbsIndexes;
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    for (auto const & country : affiliations[i])
      countryToFbsIndexes[country].emplace_back(i);
  }

  ThreadPool pool(threadsCount);
  for (auto && p : countryToFbsIndexes)
  {
    pool.SubmitWork([&, country{std::move(p.first)}, indexes{std::move(p.second)}]() {
      auto const path = base::JoinPath(temporaryMwmPath, country + DATA_FILE_EXTENSION_TMP);
      FeatureBuilderWriter<SerializationPolicy> collector(path, FileWriter::Op::OP_APPEND);
      for (auto const index : indexes)
        collector.Write(fbs[index]);
    });
  }
}

void Sort(std::vector<FeatureBuilder> & fbs)
{
  std::sort(std::begin(fbs), std::end(fbs), [](auto const & l, auto const & r) {
    auto const lGeomType = static_cast<int8_t>(l.GetGeomType());
    auto const rGeomType = static_cast<int8_t>(r.GetGeomType());

    auto const lId = l.HasOsmIds() ? l.GetMostGenericOsmId() : base::GeoObjectId();
    auto const rId = r.HasOsmIds() ? r.GetMostGenericOsmId() : base::GeoObjectId();

    auto const lPointsCount = l.GetPointsCount();
    auto const rPointsCount = r.GetPointsCount();

    auto const lKeyPoint = l.GetKeyPoint();
    auto const rKeyPoint = r.GetKeyPoint();

    return std::tie(lGeomType, lId, lPointsCount, lKeyPoint) <
           std::tie(rGeomType, rId, rPointsCount, rKeyPoint);
  });
}

bool FilenameIsCountry(std::string filename, AffiliationInterface const & affiliation)
{
  strings::ReplaceLast(filename, DATA_FILE_EXTENSION_TMP, "");
  return affiliation.HasRegionByName(filename);
}

class PlaceHelper
{
public:
  PlaceHelper() : m_table(std::make_shared<OsmIdToBoundariesTable>()), m_processor(m_table) {}

  explicit PlaceHelper(std::string const & filename) : PlaceHelper()
  {
    ForEachFromDatRawFormat<MaxAccuracy>(
        filename, [&](auto const & fb, auto const &) { m_processor.Add(fb); });
  }

  static bool IsPlace(FeatureBuilder const & fb)
  {
    auto const type = GetPlaceType(fb);
    return type != ftype::GetEmptyValue() && !fb.GetName().empty() && NeedProcessPlace(fb);
  }

  bool Process(FeatureBuilder const & fb)
  {
    m_processor.Add(fb);
    return true;
  }

  std::vector<PlaceProcessor::PlaceWithIds> GetFeatures() { return m_processor.ProcessPlaces(); }

  std::shared_ptr<OsmIdToBoundariesTable> GetTable() const { return m_table; }

private:
  std::shared_ptr<OsmIdToBoundariesTable> m_table;
  PlaceProcessor m_processor;
};

class ProcessorCities
{
public:
  ProcessorCities(std::string const & temporaryMwmPath, AffiliationInterface const & affiliation,
                  PlaceHelper & citiesHelper, size_t threadsCount = 1)
    : m_temporaryMwmPath(temporaryMwmPath)
    , m_affiliation(affiliation)
    , m_citiesHelper(citiesHelper)
    , m_threadsCount(threadsCount)
  {
  }

  void SetPromoCatalog(std::string const & filename) { m_citiesFilename = filename; }

  void Process()
  {
    std::vector<std::future<std::vector<FeatureBuilder>>> citiesResults;
    ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
      auto cities = pool.Submit([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, m_affiliation))
          return cities;

        auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);
        for (size_t i = 0; i < fbs.size(); ++i)
        {
          if (PlaceHelper::IsPlace(fbs[i]))
            cities.emplace_back(std::move(fbs[i]));
          else
            writer.Write(std::move(fbs[i]));
        }

        return cities;
      });
      citiesResults.emplace_back(std::move(cities));
    });

    for (auto & v : citiesResults)
    {
      auto const cities = v.get();
      for (auto const & city : cities)
        m_citiesHelper.Process(city);
    }
    auto fbsWithIds = m_citiesHelper.GetFeatures();
    if (!m_citiesFilename.empty())
      ProcessForPromoCatalog(fbsWithIds);

    std::vector<FeatureBuilder> fbs;
    fbs.reserve(fbsWithIds.size());
    std::transform(std::cbegin(fbsWithIds), std::cend(fbsWithIds), std::back_inserter(fbs),
                   base::RetrieveFirst());
    auto const affiliations = GetAffiliations(fbs, m_affiliation, m_threadsCount);
    AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
  }

private:
  void ProcessForPromoCatalog(std::vector<PlaceProcessor::PlaceWithIds> & fbs)
  {
    auto const cities = promo::LoadCities(m_citiesFilename);
    for (auto & fbWithIds : fbs)
    {
      for (auto const & id : fbWithIds.second)
      {
        if (cities.count(id) == 0)
          continue;

        auto static const kPromoType = classif().GetTypeByPath({"sponsored", "promo_catalog"});
        FeatureParams & params = fbWithIds.first.GetParams();
        params.AddType(kPromoType);
      }
    }
  }

  std::string m_citiesFilename;
  std::string m_temporaryMwmPath;
  AffiliationInterface const & m_affiliation;
  PlaceHelper & m_citiesHelper;
  size_t m_threadsCount;
};
}  // namespace

FinalProcessorIntermediateMwmInterface::FinalProcessorIntermediateMwmInterface(
    FinalProcessorPriority priority)
  : m_priority(priority)
{
}

bool FinalProcessorIntermediateMwmInterface::operator<(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return m_priority < other.m_priority;
}

bool FinalProcessorIntermediateMwmInterface::operator==(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return !(*this < other || other < *this);
}

bool FinalProcessorIntermediateMwmInterface::operator!=(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return !(*this == other);
}

CountryFinalProcessor::CountryFinalProcessor(std::string const & borderPath,
                                             std::string const & temporaryMwmPath,
                                             bool haveBordersForWholeWorld, size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_borderPath(borderPath)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
  , m_threadsCount(threadsCount)
{
}

void CountryFinalProcessor::SetBooking(std::string const & filename)
{
  m_hotelsFilename = filename;
}

void CountryFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
}

void CountryFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFilename = filename;
}

void CountryFinalProcessor::DumpCitiesBoundaries(std::string const & filename)
{
  m_citiesBoundariesFilename = filename;
}

void CountryFinalProcessor::DumpRoutingCitiesBoundaries(std::string const & collectorFilename,
                                                        std::string const & dumpPath)
{
  m_routingCityBoundariesCollectorFilename = collectorFilename;
  m_routingCityBoundariesDumpPath = dumpPath;
}

void CountryFinalProcessor::SetCoastlines(std::string const & coastlineGeomFilename,
                                          std::string const & worldCoastsFilename)
{
  m_coastlineGeomFilename = coastlineGeomFilename;
  m_worldCoastsFilename = worldCoastsFilename;
}

void CountryFinalProcessor::SetFakeNodes(std::string const & filename)
{
  m_fakeNodesFilename = filename;
}

void CountryFinalProcessor::SetMiniRoundabouts(std::string const & filename)
{
  m_miniRoundaboutsFilename = filename;
}

void CountryFinalProcessor::Process()
{
  if (!m_hotelsFilename.empty())
    ProcessBooking();
  if (!m_routingCityBoundariesCollectorFilename.empty())
    ProcessRoutingCityBoundaries();
  if (!m_citiesAreasTmpFilename.empty() || !m_citiesFilename.empty())
    ProcessCities();
  if (!m_coastlineGeomFilename.empty())
    ProcessCoastline();
  if (!m_miniRoundaboutsFilename.empty())
    ProcessRoundabouts();
  if (!m_fakeNodesFilename.empty())
    AddFakeNodes();

  Finish();
}

void CountryFinalProcessor::ProcessBooking()
{
  BookingDataset dataset(m_hotelsFilename);
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  {
    ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
      pool.SubmitWork([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, affiliation))
          return;

        auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);
        for (auto & fb : fbs)
        {
          auto const id = dataset.FindMatchingObjectId(fb);
          if (id == BookingHotel::InvalidObjectId())
          {
            writer.Write(fb);
          }
          else
          {
            dataset.PreprocessMatchedOsmObject(id, fb, [&](FeatureBuilder & newFeature) {
              if (newFeature.PreSerialize())
                writer.Write(newFeature);
            });
          }
        }
      });
    });
  }
  std::vector<FeatureBuilder> fbs;
  dataset.BuildOsmObjects([&](auto && fb) { fbs.emplace_back(std::move(fb)); });
  auto const affiliations = GetAffiliations(fbs, affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
}

void CountryFinalProcessor::ProcessRoundabouts()
{
  MiniRoundaboutTransformer helper(m_miniRoundaboutsFilename);

  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  {
    ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
      pool.SubmitWork([&, filename]() {
        if (!FilenameIsCountry(filename, affiliation))
          return;

        auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);

        // Adds new way features generated from mini-roundabout nodes with those nodes ids.
        // Transforms points on roads to connect them with these new roundabout junctions.
        helper.ProcessRoundabouts(affiliation, fbs);
        for (auto const & fb : fbs)
          writer.Write(fb);
      });
    });
  }
}

void CountryFinalProcessor::ProcessRoutingCityBoundaries()
{
  CHECK(
      !m_routingCityBoundariesCollectorFilename.empty() && !m_routingCityBoundariesDumpPath.empty(),
      ());

  RoutingCityBoundariesProcessor processor(m_routingCityBoundariesCollectorFilename,
                                           m_routingCityBoundariesDumpPath);
  processor.ProcessDataFromCollector();
}

void CountryFinalProcessor::ProcessCities()
{
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto citiesHelper =
      m_citiesAreasTmpFilename.empty() ? PlaceHelper() : PlaceHelper(m_citiesAreasTmpFilename);
  ProcessorCities processorCities(m_temporaryMwmPath, affiliation, citiesHelper, m_threadsCount);
  processorCities.SetPromoCatalog(m_citiesFilename);
  processorCities.Process();
  if (m_citiesBoundariesFilename.empty())
    return;

  auto const citiesTable = citiesHelper.GetTable();
  LOG(LINFO, ("Dumping cities boundaries to", m_citiesBoundariesFilename));
  SerializeBoundariesTable(m_citiesBoundariesFilename, *citiesTable);
}

void CountryFinalProcessor::ProcessCoastline()
{
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto fbs = ReadAllDatRawFormat(m_coastlineGeomFilename);
  auto const affiliations = GetAffiliations(fbs, affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
  FeatureBuilderWriter<> collector(m_worldCoastsFilename);
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    fbs[i].AddName("default", strings::JoinStrings(affiliations[i], ';'));
    collector.Write(fbs[i]);
  }
}

void CountryFinalProcessor::AddFakeNodes()
{
  std::vector<feature::FeatureBuilder> fbs;
  MixFakeNodes(m_fakeNodesFilename, [&](auto & element) {
    FeatureBuilder fb;
    fb.SetCenter(mercator::FromLatLon(element.m_lat, element.m_lon));
    fb.SetOsmId(base::MakeOsmNode(element.m_id));
    ftype::GetNameAndType(&element, fb.GetParams());
    fbs.emplace_back(std::move(fb));
  });
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto const affiliations = GetAffiliations(fbs, affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
}

void CountryFinalProcessor::Finish()
{
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  ThreadPool pool(m_threadsCount);
  ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
    pool.SubmitWork([&, filename]() {
      if (!FilenameIsCountry(filename, affiliation))
        return;

      auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
      auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
      Sort(fbs);
      FeatureBuilderWriter<> collector(fullPath);
      for (auto & fb : fbs)
        collector.Write(fb);
    });
  });
}

WorldFinalProcessor::WorldFinalProcessor(std::string const & temporaryMwmPath,
                                         std::string const & coastlineGeomFilename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_worldTmpFilename(base::JoinPath(m_temporaryMwmPath, WORLD_FILE_NAME) +
                       DATA_FILE_EXTENSION_TMP)
  , m_coastlineGeomFilename(coastlineGeomFilename)
{
}

void WorldFinalProcessor::SetPopularPlaces(std::string const & filename)
{
  m_popularPlacesFilename = filename;
}

void WorldFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
}

void WorldFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFilename = filename;
}

void WorldFinalProcessor::Process()
{
  if (!m_citiesAreasTmpFilename.empty() || !m_citiesFilename.empty())
    ProcessCities();

  auto fbs = ReadAllDatRawFormat<MaxAccuracy>(m_worldTmpFilename);
  Sort(fbs);
  WorldGenerator generator(m_worldTmpFilename, m_coastlineGeomFilename, m_popularPlacesFilename);
  for (auto & fb : fbs)
    generator.Process(fb);

  generator.DoMerge();
}

void WorldFinalProcessor::ProcessCities()
{
  auto const affiliation = SingleAffiliation(WORLD_FILE_NAME);
  auto citiesHelper =
      m_citiesAreasTmpFilename.empty() ? PlaceHelper() : PlaceHelper(m_citiesAreasTmpFilename);
  ProcessorCities processorCities(m_temporaryMwmPath, affiliation, citiesHelper);
  processorCities.SetPromoCatalog(m_citiesFilename);
  processorCities.Process();
}

CoastlineFinalProcessor::CoastlineFinalProcessor(std::string const & filename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::WorldCoasts)
  , m_filename(filename)
{
}

void CoastlineFinalProcessor::SetCoastlinesFilenames(std::string const & geomFilename,
                                                     std::string const & rawGeomFilename)
{
  m_coastlineGeomFilename = geomFilename;
  m_coastlineRawGeomFilename = rawGeomFilename;
}

void CoastlineFinalProcessor::Process()
{
  auto fbs = ReadAllDatRawFormat<MaxAccuracy>(m_filename);
  Sort(fbs);
  for (auto && fb : fbs)
    m_generator.Process(std::move(fb));

  FeaturesAndRawGeometryCollector collector(m_coastlineGeomFilename, m_coastlineRawGeomFilename);
  // Check and stop if some coasts were not merged.
  CHECK(m_generator.Finish(), ());
  LOG(LINFO, ("Generating coastline polygons."));
  size_t totalFeatures = 0;
  size_t totalPoints = 0;
  size_t totalPolygons = 0;
  std::vector<FeatureBuilder> outputFbs;
  m_generator.GetFeatures(outputFbs);
  for (auto & fb : outputFbs)
  {
    collector.Collect(fb);
    ++totalFeatures;
    totalPoints += fb.GetPointsCount();
    totalPolygons += fb.GetPolygonsCount();
  }

  LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
              "total points:", totalPoints));
}

ComplexFinalProcessor::ComplexFinalProcessor(std::string const & mwmTmpPath,
                                             std::string const & outFilename, size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::Complex)
  , m_mwmTmpPath(mwmTmpPath)
  , m_outFilename(outFilename)
  , m_threadsCount(threadsCount)
{
}

void ComplexFinalProcessor::SetGetMainTypeFunction(hierarchy::GetMainTypeFn const & getMainType)
{
  m_getMainType = getMainType;
}

void ComplexFinalProcessor::SetFilter(std::shared_ptr<FilterInterface> const & filter)
{
  m_filter = filter;
}

void ComplexFinalProcessor::SetGetNameFunction(hierarchy::GetNameFn const & getName)
{
  m_getName = getName;
}

void ComplexFinalProcessor::SetPrintFunction(hierarchy::PrintFn const & printFunction)
{
  m_printFunction = printFunction;
}

void ComplexFinalProcessor::UseCentersEnricher(std::string const & mwmPath,
                                               std::string const & osm2ftPath)
{
  m_useCentersEnricher = true;
  m_mwmPath = mwmPath;
  m_osm2ftPath = osm2ftPath;
}

std::unique_ptr<hierarchy::HierarchyEntryEnricher> ComplexFinalProcessor::CreateEnricher(
    std::string const & countryName) const
{
  return std::make_unique<hierarchy::HierarchyEntryEnricher>(
      base::JoinPath(m_osm2ftPath, countryName + DATA_FILE_EXTENSION OSM2FEATURE_FILE_EXTENSION),
      base::JoinPath(m_mwmPath, countryName + DATA_FILE_EXTENSION));
}

void ComplexFinalProcessor::UseBuildingPartsInfo(std::string const & filename)
{
  m_buildingPartsFilename = filename;
}

void ComplexFinalProcessor::Process()
{
  if (!m_buildingPartsFilename.empty())
    m_buildingToParts = std::make_unique<BuildingToBuildingPartsMap>(m_buildingPartsFilename);

  ThreadPool pool(m_threadsCount);
  std::vector<std::future<std::vector<HierarchyEntry>>> futures;
  ForEachCountry(m_mwmTmpPath, [&](auto const & filename) {
    auto future = pool.Submit([&, filename]() {
      auto countryName = filename;
      strings::ReplaceLast(countryName, DATA_FILE_EXTENSION_TMP, "");
      // https://wiki.openstreetmap.org/wiki/Simple_3D_buildings
      // An object with tag 'building:part' is a part of a relation with outline 'building' or
      // is contained in an object with tag 'building'. We will split data and work with
      // these cases separately. First of all let's remove objects with tag building:part is contained
      // in relations. We will add them back after data processing.
      std::unordered_map<base::GeoObjectId, FeatureBuilder> relationBuildingParts;

      auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(
                   base::JoinPath(m_mwmTmpPath, filename));
      if (m_buildingToParts)
        relationBuildingParts = RemoveRelationBuildingParts(fbs);

      // This case is second. We build a hierarchy using the geometry of objects and their nesting.
      auto trees = hierarchy::BuildHierarchy(std::move(fbs), m_getMainType, m_filter);

      // We remove tree roots with tag 'building:part'.
      base::EraseIf(trees, [](auto const & node) {
        static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
        return buildingPartChecker(node->GetData().GetTypes());
      });

      // We want to transform
      // building
      //        |_building-part
      //                       |_building-part
      // to
      // building
      //        |_building-part
      //        |_building-part
      hierarchy::FlattenBuildingParts(trees);
      // In the end we add objects, which were saved by the collector.
      if (m_buildingToParts)
      {
        hierarchy::AddChildrenTo(trees, [&](auto const & compositeId) {
          auto const & ids = m_buildingToParts->GetBuildingPartsByOutlineId(compositeId);
          std::vector<hierarchy::HierarchyPlace> places;
          places.reserve(ids.size());
          for (auto const & id : ids)
          {
            if (relationBuildingParts.count(id) == 0)
              continue;

            places.emplace_back(hierarchy::HierarchyPlace(relationBuildingParts[id]));
          }
          return places;
        });
      }

      // We create and save hierarchy lines.
      hierarchy::HierarchyLinesBuilder hierarchyBuilder(std::move(trees));
      if (m_useCentersEnricher)
        hierarchyBuilder.SetHierarchyEntryEnricher(CreateEnricher(countryName));

      hierarchyBuilder.SetCountry(countryName);
      hierarchyBuilder.SetGetMainTypeFunction(m_getMainType);
      hierarchyBuilder.SetGetNameFunction(m_getName);
      return hierarchyBuilder.GetHierarchyLines();
    });
    futures.emplace_back(std::move(future));
  });
  std::vector<HierarchyEntry> allLines;
  for (auto & f : futures)
  {
    auto const lines = f.get();
    allLines.insert(std::cend(allLines), std::cbegin(lines), std::cend(lines));
  }
  WriteLines(allLines);
}

std::unordered_map<base::GeoObjectId, FeatureBuilder>
ComplexFinalProcessor::RemoveRelationBuildingParts(std::vector<FeatureBuilder> & fbs)
{
  CHECK(m_buildingToParts, ());

  auto it = std::partition(std::begin(fbs), std::end(fbs), [&](auto const & fb) {
    return !m_buildingToParts->HasBuildingPart(fb.GetMostGenericOsmId());
  });

  std::unordered_map<base::GeoObjectId, FeatureBuilder> buildingParts;
  buildingParts.reserve(static_cast<size_t>(std::distance(it, std::end(fbs))));

  std::transform(it, std::end(fbs), std::inserter(buildingParts, std::begin(buildingParts)), [](auto && fb) {
    auto const id = fb.GetMostGenericOsmId();
    return std::make_pair(id, std::move(fb));
  });

  fbs.resize(static_cast<size_t>(std::distance(std::begin(fbs), it)));
  return buildingParts;
}

void ComplexFinalProcessor::WriteLines(std::vector<HierarchyEntry> const & lines)
{
  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(m_outFilename);
  for (auto const & line : lines)
    stream << m_printFunction(line) << '\n';
}
}  // namespace generator

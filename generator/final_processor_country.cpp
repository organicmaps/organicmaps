#include "generator/final_processor_country.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/isolines_generator.hpp"
#include "generator/mini_roundabout_transformer.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm2type.hpp"
#include "generator/routing_city_boundaries_processor.hpp"

#include "routing/speed_camera_prohibition.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"

#include "base/file_name_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <utility>
#include <vector>

using namespace base::thread_pool::computational;
using namespace feature;

namespace generator
{
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

void CountryFinalProcessor::SetIsolinesDir(std::string const & dir) { m_isolinesPath = dir; }

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
  if (!m_isolinesPath.empty())
    AddIsolines();
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
        auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(fullPath);
        FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(fullPath);
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
        auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(fullPath);
        FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(fullPath);

        // Adds new way features generated from mini-roundabout nodes with those nodes ids.
        // Transforms points on roads to connect them with these new roundabout junctions.
        helper.ProcessRoundabouts(affiliation, fbs);
        for (auto const & fb : fbs)
          writer.Write(fb);
      });
    });
  }
}

void CountryFinalProcessor::AddIsolines()
{
  // For generated isolines must be built isolines_info section based on the same
  // binary isolines file.
  IsolineFeaturesGenerator isolineFeaturesGenerator(m_isolinesPath);
  auto const affiliation = CountriesFilesIndexAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  ThreadPool pool(m_threadsCount);
  ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
    pool.SubmitWork([&, filename]() {
      if (!FilenameIsCountry(filename, affiliation))
        return;
      auto const countryName = GetCountryNameFromTmpMwmPath(filename);

      auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
      FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(fullPath,
                                                                     FileWriter::Op::OP_APPEND);
      isolineFeaturesGenerator.GenerateIsolines(
          countryName, [&writer](feature::FeatureBuilder && fb) { writer.Write(fb); });
    });
  });
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
      auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(fullPath);
      Sort(fbs);

      auto const speedCamerasProhibited =
          routing::AreSpeedCamerasProhibited(platform::CountryFile(filename));
      static auto const speedCameraType = classif().GetTypeByPath({"highway", "speed_camera"});

      FeatureBuilderWriter<> collector(fullPath);
      for (auto & fb : fbs)
      {
        // Removing point features with speed cameras type from geometry index for some countries.
        if (speedCamerasProhibited && fb.IsPoint() && fb.HasType(speedCameraType))
          continue;

        collector.Write(fb);
      }
    });
  });
}
}  // namespace generator

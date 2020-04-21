#include "generator/final_processor_country.hpp"

#include "generator/affiliation.hpp"
#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/isolines_generator.hpp"
#include "generator/mini_roundabout_transformer.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm2type.hpp"
#include "generator/promo_catalog_cities.hpp"
#include "generator/routing_city_boundaries_processor.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/speed_camera_prohibition.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"

#include "base/file_name_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <mutex>
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
  , m_affiliations(std::make_unique<CountriesFilesIndexAffiliation>(m_borderPath, haveBordersForWholeWorld))
  , m_threadsCount(threadsCount)
{
}

bool CountryFinalProcessor::IsCountry(std::string const & filename)
{
  return m_affiliations->HasCountryByName(filename);
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

  DropProhibitedSpeedCameras();
  Finish();
}

void CountryFinalProcessor::ProcessBooking()
{
  BookingDataset dataset(m_hotelsFilename);
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path) {
    if (!IsCountry(name))
      return;

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto & fb, auto /* pos */) {
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
    });
  }, m_threadsCount);

  std::vector<FeatureBuilder> fbs;
  dataset.BuildOsmObjects([&](auto && fb) { fbs.emplace_back(std::move(fb)); });
  AppendToMwmTmp(fbs, *m_affiliations, m_temporaryMwmPath, m_threadsCount);
}

void CountryFinalProcessor::ProcessRoundabouts()
{
  auto roundabouts = ReadDataMiniRoundabout(m_miniRoundaboutsFilename);
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path) {
    if (!IsCountry(name))
      return;

    MiniRoundaboutTransformer transformer(roundabouts.GetData(), *m_affiliations);

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto && fb, auto /* pos */) {
        if (routing::IsRoad(fb.GetTypes()) && roundabouts.RoadExists(fb))
          transformer.AddRoad(std::move(fb));
        else
          writer.Write(fb);
    });

    // Adds new way features generated from mini-roundabout nodes with those nodes ids.
    // Transforms points on roads to connect them with these new roundabout junctions.
    for (auto const & fb : transformer.ProcessRoundabouts())
      writer.Write(fb);
  }, m_threadsCount);
}

void CountryFinalProcessor::AddIsolines()
{
  // For generated isolines must be built isolines_info section based on the same
  // binary isolines file.
  IsolineFeaturesGenerator isolineFeaturesGenerator(m_isolinesPath);
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path) {
    if (!IsCountry(name))
      return;

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, FileWriter::Op::OP_APPEND);
    isolineFeaturesGenerator.GenerateIsolines(name, [&](auto const & fb) { writer.Write(fb); });
  }, m_threadsCount);
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
  auto citiesHelper =
      m_citiesAreasTmpFilename.empty() ? PlaceHelper() : PlaceHelper(m_citiesAreasTmpFilename);

  ProcessorCities processorCities(m_temporaryMwmPath, *m_affiliations, citiesHelper, m_threadsCount);
  processorCities.SetPromoCatalog(m_citiesFilename);
  processorCities.Process();

  if (!m_citiesBoundariesFilename.empty())
  {
    auto const citiesTable = citiesHelper.GetTable();
    LOG(LINFO, ("Dumping cities boundaries to", m_citiesBoundariesFilename));
    SerializeBoundariesTable(m_citiesBoundariesFilename, *citiesTable);
  }
}

void CountryFinalProcessor::ProcessCoastline()
{
  auto fbs = ReadAllDatRawFormat(m_coastlineGeomFilename);
  auto const affiliations = AppendToMwmTmp(fbs, *m_affiliations, m_temporaryMwmPath, m_threadsCount);
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
  AppendToMwmTmp(fbs, *m_affiliations, m_temporaryMwmPath, m_threadsCount);
}

void CountryFinalProcessor::DropProhibitedSpeedCameras()
{
  static auto const speedCameraType = classif().GetTypeByPath({"highway", "speed_camera"});
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path) {
    if (!IsCountry(country))
      return;

    if (!routing::AreSpeedCamerasProhibited(platform::CountryFile(country)))
      return;

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto const & fb, auto /* pos */) {
      // Removing point features with speed cameras type from geometry index for some countries.
      if (fb.IsPoint() && fb.HasType(speedCameraType))
        return;

      writer.Write(fb);
    });
  }, m_threadsCount);
}

void CountryFinalProcessor::Finish()
{
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path) {
    if (!IsCountry(country))
      return;

    auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(path);
    Sort(fbs);

    FeatureBuilderWriter<> writer(path);
    for (auto & fb : fbs)
      writer.Write(fb);

  }, m_threadsCount);
}
}  // namespace generator

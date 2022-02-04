#include "generator/final_processor_country.hpp"

#include "generator/affiliation.hpp"
#include "generator/boost_helpers.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/isolines_generator.hpp"
#include "generator/mini_roundabout_transformer.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm2type.hpp"
#include "generator/region_meta.hpp"
#include "generator/routing_city_boundaries_processor.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/speed_camera_prohibition.hpp"

#include "indexer/classificator.hpp"

#include "base/file_name_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <utility>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_RING(std::vector<m2::PointD>)

namespace generator
{
using namespace base::thread_pool::computational;
using namespace feature;

CountryFinalProcessor::CountryFinalProcessor(std::string const & borderPath,
                                             std::string const & temporaryMwmPath,
                                             std::string const & intermediateDir,
                                             bool haveBordersForWholeWorld, size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_borderPath(borderPath)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_intermediateDir(intermediateDir)
  , m_affiliations(
        std::make_unique<CountriesFilesIndexAffiliation>(m_borderPath, haveBordersForWholeWorld))
  , m_threadsCount(threadsCount)
{
}

bool CountryFinalProcessor::IsCountry(std::string const & filename)
{
  return m_affiliations->HasCountryByName(filename);
}

void CountryFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
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
  Order();

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
  ProcessBuildingParts();
  Finish();
}

void CountryFinalProcessor::Order()
{
  ForEachMwmTmp(
      m_temporaryMwmPath,
      [&](auto const & country, auto const & path) {
        if (!IsCountry(country))
          return;

        auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(path);
        generator::Order(fbs);

        FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path);
        for (auto const & fb : fbs)
          writer.Write(fb);
      },
      m_threadsCount);
}

void CountryFinalProcessor::ProcessRoundabouts()
{
  auto roundabouts = ReadDataMiniRoundabout(m_miniRoundaboutsFilename);
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path) {
    if (!IsCountry(name))
      return;

    MiniRoundaboutTransformer transformer(roundabouts.GetData(), *m_affiliations);

    RegionData data;

    if (ReadRegionData(name, data))
      transformer.SetLeftHandTraffic(data.Get(RegionData::Type::RD_DRIVING) == "l");

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

bool DoesBuildingConsistOfParts(FeatureBuilder const & fbBuilding,
                                m4::Tree<FeatureBuilder> const & buildingPartsKDTree)
{
  namespace bg = boost::geometry;
  using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;
  using BoostPolygon = bg::model::polygon<BoostPoint>;
  using BoostMultiPolygon = bg::model::multi_polygon<BoostPolygon>;

  BoostPolygon building;
  BoostMultiPolygon partsUnion;

  buildingPartsKDTree.ForEachInRect(fbBuilding.GetLimitRect(), [&](auto const & fbPart) {
    // Lazy initialization that will not occur with high probability
    if (bg::is_empty(building))
      generator::boost_helpers::FillPolygon(building, fbBuilding);

    BoostPolygon part;
    generator::boost_helpers::FillPolygon(part, fbPart);

    BoostMultiPolygon newPartsUnion;
    bg::union_(partsUnion, part, newPartsUnion);
    partsUnion = std::move(newPartsUnion);
  });

  if (bg::is_empty(building))
    return false;

  BoostMultiPolygon partsWithinBuilding;
  bg::intersection(building, partsUnion, partsWithinBuilding);

  // Consider a building as consisting of parts if the building footprint
  // is covered with parts at least by 90%.
  return bg::area(partsWithinBuilding) >= 0.9 * bg::area(building);
}

void CountryFinalProcessor::ProcessBuildingParts()
{
  static auto const & classificator = classif();
  static auto const buildingClassifType = classificator.GetTypeByPath({"building"});
  static auto const buildingPartClassifType = classificator.GetTypeByPath({"building:part"});
  static auto const buildingWithPartsClassifType = classificator.GetTypeByPath({"building", "has_parts"});

  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path) {
    if (!IsCountry(name))
      return;

    // All "building:part" features in MWM
    m4::Tree<FeatureBuilder> buildingPartsKDTree;

    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto && fb, auto /* pos */) {
      if (fb.IsArea() && fb.HasType(buildingPartClassifType))
        buildingPartsKDTree.Add(fb);
    });

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto && fb, auto /* pos */) {
      if (fb.IsArea() &&
          fb.HasType(buildingClassifType) &&
          DoesBuildingConsistOfParts(fb, buildingPartsKDTree))
      {
        fb.AddType(buildingWithPartsClassifType);
        fb.GetParams().FinishAddingTypes();
      }

      writer.Write(fb);
    });
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
    generator::Order(fbs);

    FeatureBuilderWriter<> writer(path);
    for (auto const & fb : fbs)
      writer.Write(fb);

  }, m_threadsCount);
}
}  // namespace generator

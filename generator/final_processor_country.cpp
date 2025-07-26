#include "generator/final_processor_country.hpp"

#include "generator/address_enricher.hpp"
#include "generator/addresses_collector.hpp"
#include "generator/affiliation.hpp"
#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/isolines_generator.hpp"
#include "generator/mini_roundabout_transformer.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm2type.hpp"
#include "generator/region_meta.hpp"

#include "routing/speed_camera_prohibition.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/region2d/binary_operators.hpp"

namespace generator
{
using namespace feature;

CountryFinalProcessor::CountryFinalProcessor(AffiliationInterfacePtr affiliations, std::string const & temporaryMwmPath,
                                             size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_affiliations(std::move(affiliations))
  , m_threadsCount(threadsCount)
{
  ASSERT(m_affiliations, ());
}

bool CountryFinalProcessor::IsCountry(std::string const & filename)
{
  return m_affiliations->HasCountryByName(filename);
}

void CountryFinalProcessor::Process()
{
  // Order();

  /// @todo Make "straight-way" processing. There is no need to make many functions and
  /// many read-write FeatureBuilder ops here.

  if (!m_coastlineGeomFilename.empty())
    ProcessCoastline();

  // 1. Process roundabouts and addr:interpolation first.
  if (!m_miniRoundaboutsFilename.empty() || !m_addrInterpolFilename.empty())
    ProcessRoundabouts();

  // 2. Process additional addresses then.
  if (!m_addressPath.empty())
    AddAddresses();

  if (!m_fakeNodesFilename.empty())
    AddFakeNodes();

  if (!m_isolinesPath.empty())
    AddIsolines();

  // DropProhibitedSpeedCameras();
  ProcessBuildingParts();

  // Finish();
}

/*
void CountryFinalProcessor::Order()
{
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path)
  {
    if (!IsCountry(country))
      return;

    auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(path);
    generator::Order(fbs);

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path);
    for (auto const & fb : fbs)
      writer.Write(fb);

  }, m_threadsCount);
}
*/

void CountryFinalProcessor::ProcessRoundabouts()
{
  auto const roundabouts = ReadMiniRoundabouts(m_miniRoundaboutsFilename);

  AddressesHolder addresses;
  addresses.Deserialize(m_addrInterpolFilename);

  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path)
  {
    if (!IsCountry(name))
      return;

    MiniRoundaboutTransformer transformer(roundabouts.GetData(), *m_affiliations);

    RegionData data;
    if (ReadRegionData(name, data))
      transformer.SetLeftHandTraffic(data.Get(RegionData::Type::RD_DRIVING) == "l");

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](FeatureBuilder && fb, uint64_t)
    {
      if (roundabouts.IsRoadExists(fb))
        transformer.AddRoad(std::move(fb));
      else
      {
        auto const & checker = ftypes::IsAddressInterpolChecker::Instance();
        // Check HasOsmIds() because we have non-OSM addr:interpolation FBs (Tiger).
        if (fb.IsLine() && fb.HasOsmIds() && checker(fb.GetTypes()))
        {
          if (!addresses.Update(fb))
          {
            // Not only invalid interpolation ways, but fancy buildings with interpolation type like here:
            // https://www.openstreetmap.org/#map=18/39.45672/-77.97516
            if (fb.RemoveTypesIf(checker))
              return;
          }
        }

        writer.Write(fb);
      }
    });

    // Adds new way features generated from mini-roundabout nodes with those nodes ids.
    // Transforms points on roads to connect them with these new roundabout junctions.
    transformer.ProcessRoundabouts([&writer](FeatureBuilder const & fb) { writer.Write(fb); });
  }, m_threadsCount);
}

bool DoesBuildingConsistOfParts(FeatureBuilder const & fbBuilding, m4::Tree<m2::RegionI> const & buildingPartsKDTree)
{
  m2::RegionI building;
  m2::MultiRegionI partsUnion;
  uint64_t buildingArea = 0;
  bool isError = false;

  // Make search query by native FeatureBuilder's rect.
  buildingPartsKDTree.ForEachInRect(fbBuilding.GetLimitRect(), [&](m2::RegionI const & part)
  {
    if (isError)
      return;

    // Lazy initialization that will not occur with high probability
    if (!building.IsValid())
    {
      building = coastlines_generator::CreateRegionI(fbBuilding.GetOuterGeometry());

      {
        /// @todo Patch to "normalize" region. Our relation multipolygon processor can produce regions
        /// with 2 directions (CW, CCW) simultaneously, thus area check below will fail.
        /// @see BuildingRelation, RegionArea_CommonEdges tests.
        /// Should refactor relation -> FeatureBuilder routine.
        m2::MultiRegionI rgns;
        m2::AddRegion(building, rgns);
        if (rgns.size() != 1)
          LOG(LWARNING, ("Degenerated polygon splitted:", rgns.size(), fbBuilding.DebugPrintIDs()));

        if (!rgns.empty())
        {
          // Select largest polygon.
          size_t idx = 0;
          for (size_t i = 0; i < rgns.size(); ++i)
          {
            uint64_t const s = rgns[i].CalculateArea();
            if (s > buildingArea)
            {
              buildingArea = s;
              idx = i;
            }
          }
          building.Swap(rgns[idx]);
        }
        else
        {
          isError = true;
          building = {};
        }
      }
    }

    // Take parts that smaller than input building outline.
    // Example of a big building:part as a "stylobate" here:
    // https://www.openstreetmap.org/way/533683349#map=18/53.93091/27.65261
    if (0.8 * part.CalculateArea() <= buildingArea)
      m2::AddRegion(part, partsUnion);
  });

  if (!building.IsValid())
    return false;

  uint64_t const isectArea = m2::Area(m2::IntersectRegions(building, partsUnion));
  // That doesn't work with *very* degenerated polygons like https://www.openstreetmap.org/way/629725974.
  // CHECK(isectArea * 0.95 <= buildingArea, (isectArea, buildingArea, fbBuilding.DebugPrintIDs()));

  // Consider building as consisting of parts if the building footprint is covered with parts at least by 90%.
  return isectArea >= 0.9 * buildingArea;
}

void CountryFinalProcessor::ProcessBuildingParts()
{
  auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
  auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
  auto const & buildingHasPartsChecker = ftypes::IsBuildingHasPartsChecker::Instance();

  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path)
  {
    if (!IsCountry(name))
      return;

    // All "building:part" regions in MWM
    m4::Tree<m2::RegionI> buildingPartsKDTree;

    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](FeatureBuilder && fb, uint64_t)
    {
      if (fb.IsArea() && buildingPartChecker(fb.GetTypes()))
      {
        // Important trick! Add region by FeatureBuilder's native rect, to make search queries also by FB rects.
        buildingPartsKDTree.Add(coastlines_generator::CreateRegionI(fb.GetOuterGeometry()), fb.GetLimitRect());
      }
    });

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](FeatureBuilder && fb, uint64_t)
    {
      if (fb.IsArea() && buildingChecker(fb.GetTypes()) && DoesBuildingConsistOfParts(fb, buildingPartsKDTree))
      {
        fb.AddType(buildingHasPartsChecker.GetType());
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
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path)
  {
    if (!IsCountry(name))
      return;

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, FileWriter::Op::OP_APPEND);
    isolineFeaturesGenerator.GenerateIsolines(name, [&](auto const & fb) { writer.Write(fb); });
  }, m_threadsCount);
}

void CountryFinalProcessor::AddAddresses()
{
  AddressEnricher::Stats totalStats;

  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & name, auto const & path)
  {
    if (!IsCountry(name))
      return;

    auto const addrPath = base::JoinPath(m_addressPath, name) + TEMP_ADDR_EXTENSION;
    if (!Platform::IsFileExistsByFullPath(addrPath))
      return;

    AddressEnricher enricher;

    // Collect existing addresses and streets.
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(
        path, [&](FeatureBuilder && fb, uint64_t) { enricher.AddSrc(std::move(fb)); });

    // Append new addresses.
    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, FileWriter::Op::OP_APPEND);
    enricher.ProcessRawEntries(addrPath, [&writer](FeatureBuilder const & fb) { writer.Write(fb); });

    LOG(LINFO, (name, enricher.m_stats));
    totalStats.Add(enricher.m_stats);
  });

  LOG(LINFO, ("Total addresses:", totalStats));
}

void CountryFinalProcessor::ProcessCoastline()
{
  /// @todo We can remove MinSize at all.
  auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(m_coastlineGeomFilename);

  auto const affiliations = AppendToMwmTmp(fbs, *m_affiliations, m_temporaryMwmPath, m_threadsCount);
  FeatureBuilderWriter<> collector(m_worldCoastsFilename);
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    fbs[i].SetName(StringUtf8Multilang::kDefaultCode, strings::JoinStrings(affiliations[i], ';'));
    collector.Write(fbs[i]);
  }
}

void CountryFinalProcessor::AddFakeNodes()
{
  std::vector<FeatureBuilder> fbs;
  MixFakeNodes(m_fakeNodesFilename, [&](auto & element)
  {
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
  auto const speedCameraType = classif().GetTypeByPath({"highway", "speed_camera"});
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path)
  {
    if (!IsCountry(country))
      return;

    if (!routing::AreSpeedCamerasProhibited(platform::CountryFile(country)))
      return;

    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](FeatureBuilder const & fb, uint64_t)
    {
      // Removing point features with speed cameras type from geometry index for some countries.
      if (fb.IsPoint() && fb.HasType(speedCameraType))
        return;

      writer.Write(fb);
    });
  }, m_threadsCount);
}

/*
void CountryFinalProcessor::Finish()
{
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path)
  {
    if (!IsCountry(country))
      return;

    auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(path);
    generator::Order(fbs);

    FeatureBuilderWriter<> writer(path);
    for (auto const & fb : fbs)
      writer.Write(fb);

  }, m_threadsCount);
}
*/

}  // namespace generator

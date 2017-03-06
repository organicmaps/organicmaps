#include "testing/testing.hpp"

#include "routing/cross_mwm_road_graph.hpp"
#include "routing/cross_routing_context.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/routing_mapping.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/index.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"
#include "base/random.hpp"

#include "std/limits.hpp"

#include <chrono>

using namespace routing;

namespace
{
UNIT_TEST(CheckCrossSections)
{
  static double constexpr kPointEquality = 0.01;
  vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                       localFiles);

  size_t ingoingErrors = 0;
  size_t outgoingErrors = 0;
  size_t noRouting = 0;
  for (auto & file : localFiles)
  {
    LOG(LINFO, ("Checking cross table for country:", file.GetCountryName()));
    CrossRoutingContextReader crossReader;

    file.SyncWithDisk();
    if (file.GetFiles() != MapOptions::MapWithCarRouting)
    {
      noRouting++;
      LOG(LINFO, ("Warning! Routing file not found for:", file.GetCountryName()));
      continue;
    }
    FilesMappingContainer container(file.GetPath(MapOptions::CarRouting));
    if (!container.IsExist(ROUTING_CROSS_CONTEXT_TAG))
    {
      noRouting++;
      LOG(LINFO, ("Warning! Mwm file has not cross mwm section:", file.GetCountryName()));
      continue;
    }
    crossReader.Load(container.GetReader(ROUTING_CROSS_CONTEXT_TAG));

    bool error = false;
    crossReader.ForEachIngoingNode([&error](IngoingCrossNode const & node)
                                   {
                                     if (node.m_point.EqualDxDy(ms::LatLon::Zero(), kPointEquality))
                                       error = true;
                                   });
    if (error)
      ingoingErrors++;

    error = false;
    crossReader.ForEachOutgoingNode([&error](OutgoingCrossNode const & node)
                                    {
                                      if (node.m_point.EqualDxDy(ms::LatLon::Zero(), kPointEquality))
                                        error = true;
                                    });
    if (error)
      outgoingErrors++;
  }
  TEST_EQUAL(ingoingErrors, 0, ("Some countries have zero point incomes."));
  TEST_EQUAL(outgoingErrors, 0, ("Some countries have zero point exits."));
  LOG(LINFO, ("Found", localFiles.size(), "countries.",
              noRouting, "countries are without routing file."));
}

UNIT_TEST(CheckOsrmToFeatureMapping)
{
  vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                       localFiles);

  size_t errors = 0;
  size_t checked = 0;
  for (auto & file : localFiles)
  {
    LOG(LINFO, ("Checking nodes for country:", file.GetCountryName()));

    file.SyncWithDisk();
    if (file.GetFiles() != MapOptions::MapWithCarRouting)
    {
      LOG(LINFO, ("Warning! Routing file not found for:", file.GetCountryName()));
      continue;
    }

    FilesMappingContainer container(file.GetPath(MapOptions::CarRouting));
    if (!container.IsExist(ROUTING_FTSEG_FILE_TAG))
    {
      LOG(LINFO, ("Warning! Mwm file has not routing ftseg section:", file.GetCountryName()));
      continue;
    }
    ++checked;

    routing::TDataFacade dataFacade;
    dataFacade.Load(container);

    OsrmFtSegMapping segMapping;
    segMapping.Load(container, file);
    segMapping.Map(container);

    for (size_t i = 0; i < dataFacade.GetNumberOfNodes(); ++i)
    {
      buffer_vector<OsrmMappingTypes::FtSeg, 8> buffer;
      segMapping.ForEachFtSeg(i, MakeBackInsertFunctor(buffer));
      if (buffer.empty())
      {
        LOG(LINFO, ("Error found in ", file.GetCountryName()));
        errors++;
        break;
      }
    }
  }
  LOG(LINFO, ("Found", localFiles.size(), "countries. In", checked, "maps with routing have", errors, "with errors."));
  TEST_EQUAL(errors, 0, ("Some countries have osrm and features mismatch."));
}

// The idea behind the test is
// 1. to go through all ingoing nodes of all mwms
// 2. to find all cross mwm outgoing edges for each ingoing node
// 3. to find all edges which are ingoing for the outgoing edges
// 4. to check that an edge mentioned in (1) there's between the ingoing edges
// Note. This test may take more than 3 hours.
UNIT_TEST(CrossMwmGraphTest)
{
  vector<platform::LocalCountryFile> localFiles;
  Index index;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                       localFiles);

  for (platform::LocalCountryFile & file : localFiles)
  {
    file.SyncWithDisk();
    index.RegisterMap(file);
  }

  for (auto it = localFiles.begin(); it != localFiles.end();)
  {
    string const & countryName = it->GetCountryName();
    MwmSet::MwmId const mwmId = index.GetMwmIdByCountryFile(it->GetCountryFile());
    if (countryName == "minsk-pass" || mwmId.GetInfo()->GetType() != MwmInfo::COUNTRY)
      it = localFiles.erase(it);
    else
      ++it;
  }

  Platform p;
  unique_ptr<storage::CountryInfoGetter> infoGetter =
      storage::CountryInfoReader::CreateCountryInfoReader(p);
  auto countryFileGetter = [&infoGetter](m2::PointD const & pt) {
    return infoGetter->GetRegionCountryId(pt);
  };

  RoutingIndexManager manager(countryFileGetter, index);
  CrossMwmGraph crossMwmGraph(manager);

  auto const seed = std::chrono::system_clock::now().time_since_epoch().count();
  LOG(LINFO, ("Seed for RandomSample:", seed));
  std::minstd_rand rng(static_cast<unsigned int>(seed));
  std::vector<size_t> subset = base::RandomSample(localFiles.size(), 10 /* mwm number */, rng);
  std::vector<platform::LocalCountryFile> subsetCountryFiles;
  for (size_t i : subset)
    subsetCountryFiles.push_back(localFiles[i]);

  for (platform::LocalCountryFile const & file : subsetCountryFiles)
  {
    string const & countryName = file.GetCountryName();
    LOG(LINFO, ("Processing", countryName));
    MwmSet::MwmId const mwmId = index.GetMwmIdByCountryFile(file.GetCountryFile());

    TEST(mwmId.IsAlive(), ("Mwm name:", countryName, "Subset:", subsetCountryFiles));
    TRoutingMappingPtr currentMapping = manager.GetMappingById(mwmId);
    if (!currentMapping->IsValid())
      continue;  // No routing sections in the mwm.

    currentMapping->LoadCrossContext();
    currentMapping->FreeFileIfPossible();
    CrossRoutingContextReader const & currentContext = currentMapping->m_crossContext;

    size_t ingoingCounter = 0;
    currentContext.ForEachIngoingNode([&](IngoingCrossNode const & node) {
      ++ingoingCounter;
      vector<BorderCross> const & targets =
          crossMwmGraph.ConstructBorderCross(currentMapping, node);
      for (BorderCross const & t : targets)
      {
        vector<CrossWeightedEdge> outAdjs;
        crossMwmGraph.GetOutgoingEdgesList(t, outAdjs);
        for (CrossWeightedEdge const & out : outAdjs)
        {
          vector<CrossWeightedEdge> inAdjs;
          crossMwmGraph.GetIngoingEdgesList(out.GetTarget(), inAdjs);
          TEST(find_if(inAdjs.cbegin(), inAdjs.cend(),
                       [&](CrossWeightedEdge const & e) {
                         return e.GetTarget() == t && out.GetWeight() == e.GetWeight();
                       }) != inAdjs.cend(),
               ("ForEachOutgoingNodeNearPoint() and ForEachIngoingNodeNearPoint() arn't "
                "correlated. Mwm:",
                countryName, "Subset:", subsetCountryFiles));
        }
      }
    });

    size_t outgoingCounter = 0;
    currentContext.ForEachOutgoingNode(
        [&](OutgoingCrossNode const & /* node */) { ++outgoingCounter; });

    LOG(LINFO, ("Processed:", countryName, "Exits:", outgoingCounter, "Enters:", ingoingCounter));
  }
}
}  // namespace

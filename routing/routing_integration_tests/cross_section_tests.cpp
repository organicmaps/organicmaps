#include "testing/testing.hpp"

#include "routing/cross_routing_context.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/routing_mapping.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include "std/limits.hpp"

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
}  // namespace

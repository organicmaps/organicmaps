#include "testing/testing.hpp"

#include "routing/cross_routing_context.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/point2d.hpp"

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
}  // namespace

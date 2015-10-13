#include "testing/testing.hpp"

#include "routing/cross_routing_context.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

using namespace routing;

namespace
{
void CheckCrosses()
{
  static double constexpr kPointEquality = 0.01;
  vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMaps(localFiles);

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
      LOG(LINFO, ("Warning! Routing file not found!"));
      continue;
    }
    FilesMappingContainer container(file.GetPath(MapOptions::CarRouting));
    crossReader.Load(container.GetReader(ROUTING_CROSS_CONTEXT_TAG));
    auto ingoing = crossReader.GetIngoingIterators();
    for (auto i = ingoing.first; i != ingoing.second; ++i)
    {
      if (my::AlmostEqualAbs(i->m_point.x, 0.0, kPointEquality) &&
          my::AlmostEqualAbs(i->m_point.y, 0.0, kPointEquality))
      {
        ingoingErrors++;
        break;
      }
    }
    auto outgoing = crossReader.GetOutgoingIterators();
    for (auto i = outgoing.first; i != outgoing.second; ++i)
    {
      if (my::AlmostEqualAbs(i->m_point.x, 0.0, kPointEquality) &&
          my::AlmostEqualAbs(i->m_point.y, 0.0, kPointEquality))
      {
        outgoingErrors++;
        break;
      }
    }
  }
  TEST_EQUAL(ingoingErrors + outgoingErrors, 0, ("Some countries have zero point exits."));
  LOG(LINFO, ("Found ", localFiles.size(), "countries.", noRouting, "has no routing file."));
}

UNIT_TEST(CheckCrossSections)
{
  CheckCrosses();
}
}  // namespace

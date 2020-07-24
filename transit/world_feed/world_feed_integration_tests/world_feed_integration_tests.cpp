#include "testing/testing.hpp"

#include "generator/affiliation.hpp"

#include "transit/world_feed/world_feed.hpp"

#include "platform/platform.hpp"

#include "coding/zip_reader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace
{
// You can download this archive to current directory by running:
// rsync -v -p testdata.mapsme.cloud.devmail.ru::testdata/gtfs-feeds-for-tests.zip .
std::string const kArchiveWithFeeds = "gtfs-feeds-for-tests";
}  // namespace

namespace transit
{
class WorldFeedIntegrationTests
{
public:
  WorldFeedIntegrationTests()
    : m_mwmMatcher(GetTestingOptions().m_resourcePath, false /* haveBordersForWholeWorld */)
    , m_globalFeed(m_generator, m_colorPicker, m_mwmMatcher)
  {
    auto const & options = GetTestingOptions();

    GetPlatform().SetResourceDir(options.m_resourcePath);
    m_testPath = base::JoinPath(GetPlatform().WritableDir(), kArchiveWithFeeds);
    CHECK(GetPlatform().MkDirRecursively(m_testPath), ());

    m_generator = IdGenerator(base::JoinPath(m_testPath, "mapping.txt"));

    auto const src = base::JoinPath(options.m_resourcePath, kArchiveWithFeeds + ".zip");
    ZipFileReader::FileList filesAndSizes;
    ZipFileReader::FilesList(src, filesAndSizes);

    for (auto const & fileAndSize : filesAndSizes)
    {
      auto const output = base::JoinPath(m_testPath, fileAndSize.first);
      if (strings::EndsWith(output, base::GetNativeSeparator()))
        CHECK(GetPlatform().MkDirRecursively(output), ());
      else
        ZipFileReader::UnzipFile(src, fileAndSize.first, output);
    }
  }

  ~WorldFeedIntegrationTests() { CHECK(Platform::RmDirRecursively(m_testPath), ()); }

  void ReadMinimalisticFeed()
  {
    gtfs::Feed feed(base::JoinPath(m_testPath, "minimalistic_feed"));
    TEST_EQUAL(feed.read_feed().code, gtfs::ResultCode::OK, ());
    TEST(m_globalFeed.SetFeed(std::move(feed)), ());

    TEST_EQUAL(m_globalFeed.m_networks.m_data.size(), 1, ());
    TEST_EQUAL(m_globalFeed.m_routes.m_data.size(), 2, ());
    // We check that lines with no entries in stop_times.txt are not added.
    TEST_EQUAL(m_globalFeed.m_lines.m_data.size(), 2, ());
    TEST_EQUAL(m_globalFeed.m_stops.m_data.size(), 7, ());
    TEST_EQUAL(m_globalFeed.m_shapes.m_data.size(), 2, ());
    // There are 2 lines with 3 and 4 stops correspondingly. So we have 5 edges.
    TEST_EQUAL(m_globalFeed.m_edges.m_data.size(), 5, ());
  }

  void ReadRealLifeFeed()
  {
    gtfs::Feed feed(base::JoinPath(m_testPath, "real_life_feed"));
    TEST_EQUAL(feed.read_feed().code, gtfs::ResultCode::OK, ());
    TEST(m_globalFeed.SetFeed(std::move(feed)), ());

    TEST_EQUAL(m_globalFeed.m_networks.m_data.size(), 21, ());
    TEST_EQUAL(m_globalFeed.m_routes.m_data.size(), 87, ());
    // All trips have unique service_id so each line corresponds to some trip.
    TEST_EQUAL(m_globalFeed.m_lines.m_data.size(), 980, ());
    TEST_EQUAL(m_globalFeed.m_stops.m_data.size(), 1008, ());
    // 64 shapes contained in other shapes should be skipped.
    TEST_EQUAL(m_globalFeed.m_shapes.m_data.size(), 329, ());
    TEST_EQUAL(m_globalFeed.m_gates.m_data.size(), 0, ());
    TEST_EQUAL(m_globalFeed.m_transfers.m_data.size(), 0, ());
    TEST_EQUAL(m_globalFeed.m_edges.m_data.size(), 10079, ());
    TEST_EQUAL(m_globalFeed.m_edgesTransfers.m_data.size(), 0, ());
  }

  // Test for train itinerary that passes through 4 regions in Europe and consists of 4 stops
  // (each in separate mwm) and 1 route with 1 line. This line passes through 4 stops:
  // [1] Switzerland_Ticino -> [2] Switzerland_Eastern ->
  // [3] Italy_Lombardy_Como -> [4] Italy_Lombardy_Monza and Brianza
  void SplitFeedIntoMultipleRegions()
  {
    gtfs::Feed feed(base::JoinPath(m_testPath, "feed_long_itinerary"));
    TEST_EQUAL(feed.read_feed().code, gtfs::ResultCode::OK, ());
    TEST(m_globalFeed.SetFeed(std::move(feed)), ());

    TEST_EQUAL(m_globalFeed.m_lines.m_data.size(), 1, ());
    TEST_EQUAL(m_globalFeed.m_stops.m_data.size(), 4, ());
    TEST_EQUAL(m_globalFeed.m_edges.m_data.size(), 3, ());

    m_globalFeed.SplitFeedIntoRegions();

    size_t const mwmCount = 4;
    // We check that count of keys in each regions-to-ids mapping corresponds to the mwms count.
    TEST_EQUAL(m_globalFeed.m_splitting.m_networks.size(), mwmCount, ());
    TEST_EQUAL(m_globalFeed.m_splitting.m_routes.size(), mwmCount, ());
    TEST_EQUAL(m_globalFeed.m_splitting.m_lines.size(), mwmCount, ());
    TEST_EQUAL(m_globalFeed.m_splitting.m_stops.size(), mwmCount, ());

    auto & stopsInRegions = m_globalFeed.m_splitting.m_stops;
    auto & edgesInRegions = m_globalFeed.m_splitting.m_edges;

    // First and last stops are connected through 1 edge with 1 nearest stop.
    // Stops in the middle are connected through 2 edges with 2 nearest stops.
    TEST_EQUAL(stopsInRegions["Switzerland_Ticino"].size(), 2, ());
    TEST_EQUAL(edgesInRegions["Switzerland_Ticino"].size(), 1, ());

    TEST_EQUAL(stopsInRegions["Switzerland_Eastern"].size(), 3, ());
    TEST_EQUAL(edgesInRegions["Switzerland_Eastern"].size(), 2, ());

    TEST_EQUAL(stopsInRegions["Italy_Lombardy_Como"].size(), 3, ());
    TEST_EQUAL(edgesInRegions["Italy_Lombardy_Como"].size(), 2, ());

    TEST_EQUAL(stopsInRegions["Italy_Lombardy_Monza and Brianza"].size(), 2, ());
    TEST_EQUAL(edgesInRegions["Italy_Lombardy_Monza and Brianza"].size(), 1, ());
  }

private:
  std::string m_testPath;
  IdGenerator m_generator;
  transit::ColorPicker m_colorPicker;
  feature::CountriesFilesAffiliation m_mwmMatcher;
  WorldFeed m_globalFeed;
};

UNIT_CLASS_TEST(WorldFeedIntegrationTests, MinimalisticFeed)
{
  ReadMinimalisticFeed();
}

UNIT_CLASS_TEST(WorldFeedIntegrationTests, RealLifeFeed)
{
  ReadRealLifeFeed();
}

UNIT_CLASS_TEST(WorldFeedIntegrationTests, FeedWithLongItinerary)
{
  SplitFeedIntoMultipleRegions();
}
}  // namespace transit

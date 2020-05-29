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
    TEST_EQUAL(m_globalFeed.m_lines.m_data.size(), 981, ());
    TEST_EQUAL(m_globalFeed.m_stops.m_data.size(), 1021, ());
    // 64 shapes contained in other shapes should be skipped.
    TEST_EQUAL(m_globalFeed.m_shapes.m_data.size(), 329, ());
    TEST_EQUAL(m_globalFeed.m_gates.m_data.size(), 0, ());
    TEST_EQUAL(m_globalFeed.m_transfers.m_data.size(), 0, ());
    TEST_EQUAL(m_globalFeed.m_edges.m_data.size(), 10091, ());
    TEST_EQUAL(m_globalFeed.m_edgesTransfers.m_data.size(), 0, ());
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
  WorldFeedIntegrationTests::ReadMinimalisticFeed();
}

UNIT_CLASS_TEST(WorldFeedIntegrationTests, RealLifeFeed)
{
  WorldFeedIntegrationTests::ReadRealLifeFeed();
}
}  // namespace transit

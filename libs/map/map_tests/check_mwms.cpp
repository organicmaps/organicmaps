#include "testing/testing.hpp"

#include "map/features_fetcher.hpp"

#include "indexer/data_header.hpp"
#include "indexer/scale_index.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <map>
#include <vector>

#include "defines.hpp"

namespace check_mwms
{
using namespace platform;
using namespace std;

UNIT_TEST(CheckMWM_LoadAll)
{
  // Parse root ResourcesDir folder. Expect at least World, WorldCoasts, minsk-pass.
  vector<LocalCountryFile> localFiles;
  size_t const count = FindAllLocalMapsInDirectoryAndCleanup(GetPlatform().ResourcesDir(), 0 /* version */,
                                                             -1 /* latestVersion */, localFiles);
  TEST_EQUAL(count, localFiles.size(), ());
  TEST_GREATER_OR_EQUAL(count, 3, ());

  FeaturesFetcher m;
  m.InitClassificator();

  for (auto const & localFile : localFiles)
  {
    LOG(LINFO, ("Found mwm:", localFile));
    try
    {
      auto p = m.RegisterMap(localFile);

      TEST(p.first.IsAlive(), ());
      TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());
    }
    catch (RootException const & ex)
    {
      TEST(false, ("Bad mwm file:", localFile));
    }
  }
}

UNIT_TEST(CheckMWM_GeomIndex)
{
  // Open mwm file from data path.
  FilesContainerR cont(GetPlatform().GetReader("minsk-pass.mwm"));

  // Initialize index reader section inside mwm.
  ScaleIndex<ModelReaderPtr> const index(cont.GetReader(INDEX_FILE_TAG));

  // Pass full coverage as input for test.
  uint64_t beg = 0;
  uint64_t end = static_cast<uint64_t>((1ULL << 63) - 1);

  // Count objects visible on each scale.
  map<int, uint64_t> resCount;
  int const lastScale = feature::DataHeader(cont).GetLastScale();
  for (int scale = 0; scale <= lastScale; ++scale)
    index.ForEachInIntervalAndScale(beg, end, scale, [scale, &resCount](uint32_t) { ++resCount[scale]; });

  // A scale has the objects of the previous ones.
  TEST_GREATER(resCount[lastScale], 0, ());
  for (int scale = 1; scale <= lastScale; ++scale)
    TEST_GREATER_OR_EQUAL(resCount[scale], resCount[scale - 1], (scale));

  // Print results.
  LOG(LINFO, (resCount));
}
}  // namespace check_mwms

#include "testing/testing.hpp"

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/read_metaline_task.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "platform/local_country_file.hpp"

#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace read_metaline_task_tests
{
size_t constexpr kNever = std::numeric_limits<size_t>::max();

// Reads the metalines of the test map, which is deregistered (like on its update or deletion)
// before the features reading number |deregisterOnRead|.
// Returns the numbers of the features readings and of the features with a metaline.
std::pair<size_t, size_t> ReadMetalines(size_t deregisterOnRead)
{
  classificator::Load();

  auto const file = platform::LocalCountryFile::MakeForTesting("minsk-pass");
  FrozenDataSource dataSource;
  auto const [mwmId, regResult] = dataSource.RegisterMap(file);
  TEST_EQUAL(regResult, MwmSet::RegResult::Success, ());

  size_t readsCount = 0;
  df::MapDataProvider model([](auto const &, auto const &, int) {},
                            [&](auto const & fn, std::vector<FeatureID> const & ids)
  {
    if (readsCount++ == deregisterOnRead)
      TEST(dataSource.DeregisterMap(file.GetCountryFile()), ());
    dataSource.ReadFeatures(fn, ids);
  }, [](std::string_view) { return true; }, [](auto const &, int) {}, [](auto const &, auto) { return false; },
                            [](auto const &, auto) {}, [](auto const &) { return false; }, [](auto const &...) {});

  df::ReadMetalineTask task(model, mwmId);
  task.Run();

  df::MetalineCache cache;
  task.UpdateCache(cache);
  return {readsCount, cache.size()};
}

UNIT_TEST(ReadMetalineTask_MwmDeregistered)
{
  auto const [allReads, allFeatures] = ReadMetalines(kNever);
  TEST_GREATER(allReads, 2, ());
  TEST_GREATER(allFeatures, 0, ());

  // The task stops at the first reading of the deregistered map.
  auto const [reads0, features0] = ReadMetalines(0 /* deregisterOnRead */);
  TEST_EQUAL(reads0, 1, ());
  TEST_EQUAL(features0, 0, ());

  // The metalines which were read before are dropped.
  auto const [reads1, features1] = ReadMetalines(1 /* deregisterOnRead */);
  TEST_EQUAL(reads1, 2, ());
  TEST_EQUAL(features1, 0, ());
}
}  // namespace read_metaline_task_tests

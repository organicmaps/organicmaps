#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/features_offsets_table.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace concurrent_feature_parsing_test
{
using namespace platform;
using namespace std;

void TestConcurrentAccessToFeatures(string const & mwm)
{
  FrozenDataSource dataSource;

  auto const & countryFile = CountryFile(mwm);
  auto const & local = integration::GetLocalCountryFileByCountryId(countryFile);
  TEST_NOT_EQUAL(local, LocalCountryFile(), ());
  TEST(local.HasFiles(), (local));

  auto const mwmIdAndRegResult = dataSource.RegisterMap(local);
  TEST_EQUAL(mwmIdAndRegResult.second, MwmSet::RegResult::Success, (local));
  TEST(mwmIdAndRegResult.first.IsAlive(), (local));

  auto const handle = dataSource.GetMwmHandleByCountryFile(countryFile);
  TEST(handle.IsAlive(), (local));

  auto const featureNumber = handle.GetValue()->m_ftTable->size();
  // Note. If hardware_concurrency() returns 0 it means that number of core is not defined.
  // If hardware_concurrency() returns 1 it means that only one core is available.
  // In the both cases 2 threads should be used for this test.
  auto const threadNumber = max(thread::hardware_concurrency(), static_cast<unsigned int>(2));
  LOG(LINFO, ("Parsing geometry of", featureNumber, "features in", threadNumber, "threads simultaneously.", local));

  mutex guardCtorMtx;
  auto parseGeometries = [&](vector<m2::PointD> & points)
  {
    unique_lock<mutex> guardCtor(guardCtorMtx);
    FeaturesLoaderGuard guard(dataSource, handle.GetId());
    guardCtor.unlock();

    for (uint32_t i = 0; i < featureNumber; ++i)
    {
      auto feature = guard.GetFeatureByIndex(i);
      TEST(feature, ("Feature id:", i, "is not found in", local));

      feature->ParseGeometry(FeatureType::BEST_GEOMETRY);
      for (size_t i = 0; i < feature->GetPointsCount(); ++i)
        points.push_back(feature->GetPoint(i));
    }
  };

  vector<future<void>> futures;
  futures.reserve(threadNumber);
  vector<vector<m2::PointD>> pointsByThreads;
  pointsByThreads.resize(threadNumber);
  for (size_t i = 0; i + 1 < threadNumber; ++i)
    futures.push_back(async(launch::async, parseGeometries, ref(pointsByThreads[i])));
  parseGeometries(pointsByThreads[threadNumber - 1]);

  for (auto const & fut : futures)
    fut.wait();

  // Checking that all geometry points are equal after parsing in different threads.
  CHECK_GREATER_OR_EQUAL(pointsByThreads.size(), 2, ());
  for (size_t i = 1; i < pointsByThreads.size(); ++i)
  {
    TEST_EQUAL(pointsByThreads[i].size(), pointsByThreads[0].size(), (i));
    for (size_t j = 0; j < pointsByThreads[i].size(); ++j)
      TEST_EQUAL(pointsByThreads[i][j], pointsByThreads[0][j], (i, j));
  }

  LOG(LINFO, ("Parsing is done."));
}

// This test on availability of parsing FeatureType in several threads.
UNIT_TEST(ConcurrentFeatureParsingTest)
{
  classificator::Load();

  vector<string> const mwms = {"Russia_Moscow", "Nepal_Kathmandu", "Netherlands_North Holland_Amsterdam"};

  for (auto const & mwm : mwms)
    TestConcurrentAccessToFeatures(mwm);
}
}  // namespace concurrent_feature_parsing_test

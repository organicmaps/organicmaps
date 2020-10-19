#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
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

  auto const featureNumber = handle.GetValue()->m_table->size();
  // Note. If hardware_concurrency() returns 0 it means that number of core is not defined.
  // If hardware_concurrency() returns 1 it means that only one core is available.
  // In the both cases 2 threads should be used for this test.
  auto const threadNumber = max(thread::hardware_concurrency(), static_cast<unsigned int>(2));
  LOG(LINFO, ("Parsing geometry of", featureNumber, "features in", threadNumber,
              "threads simultaneously.", local));

  mutex guardCtorMtx;
  auto parseGeometries = [&guardCtorMtx, &featureNumber,  &dataSource, &handle, &local](){
    unique_lock<mutex> guardCtor(guardCtorMtx);
    FeaturesLoaderGuard guard(dataSource, handle.GetId());
    guardCtor.unlock();

    for (uint32_t i = 0; i < featureNumber; ++i)
    {
      auto feature = guard.GetFeatureByIndex(i);
      TEST(feature, ("Feature id:", i, "is not found in", local));

      feature->ParseGeometry(FeatureType::BEST_GEOMETRY);
    }
  };

  vector<future<void>> futures;
  futures.reserve(threadNumber);
  for (size_t i = 0; i < threadNumber - 1; ++i)
    futures.push_back(std::async(std::launch::async, parseGeometries));
  parseGeometries();

  for (auto const & fut : futures)
    fut.wait();

  LOG(LINFO, ("Parsing is done."));
}

// This test on availability of parsing FeatureType in several threads.
UNIT_TEST(ConcurrentFeatureParsingTest)
{
  classificator::Load();

  vector<string> const mwms = {"Russia_Moscow", "Nepal_Kathmandu",
                               "Netherlands_North Holland_Amsterdam"};

  for (auto const & mwm : mwms)
    TestConcurrentAccessToFeatures(mwm);
}
}  // namespace

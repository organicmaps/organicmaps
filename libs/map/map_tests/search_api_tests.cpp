// NOTE: the purpose of this test is to test interaction between
// SearchAPI and search engine. If you would like to test search
// engine behaviour, please, consider implementing another search
// integration test.

#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "search/address_matcher.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "map/bookmarks_search_params.hpp"
#include "map/search_api.hpp"
#include "map/viewport_search_params.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/scales.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace search_api_tests
{
using namespace search::tests_support;
using namespace generator::tests_support;
using namespace search;
using namespace std;
using namespace storage;

namespace
{
using Rules = vector<shared_ptr<MatchingRule>>;

struct TestCafe : public TestPOI
{
public:
  TestCafe(m2::PointD const & center, string const & name, string const & lang) : TestPOI(center, name, lang)
  {
    SetTypes({{"amenity", "cafe"}});
  }

  ~TestCafe() override = default;
};

class Delegate : public SearchAPI::Delegate
{
public:
  ~Delegate() override = default;

  // SearchAPI::Delegate overrides:
  void RunUITask(function<void()> fn) override { fn(); }
};

class SearchAPITest : public generator::tests_support::TestWithCustomMwms
{
public:
  SearchAPITest()
    : m_infoGetter(CountryInfoReader::CreateCountryInfoGetter(GetPlatform()))
    , m_api(m_dataSource, m_storage, *m_infoGetter, 1 /* numThreads */, m_delegate)
  {}

protected:
  Storage m_storage;
  unique_ptr<CountryInfoGetter> m_infoGetter;
  Delegate m_delegate;
  SearchAPI m_api;
};

class QueuedDelegate : public SearchAPI::Delegate
{
public:
  void RunUITask(function<void()> fn) override
  {
    {
      lock_guard lock(m_mutex);
      m_tasks.push_back(std::move(fn));
    }
    m_condition.notify_one();
  }

  bool WaitUntil(function<bool()> const & finished)
  {
    auto const deadline = chrono::steady_clock::now() + chrono::seconds(10);
    while (!finished())
    {
      function<void()> task;
      {
        unique_lock lock(m_mutex);
        if (!m_condition.wait_until(lock, deadline, [this] { return !m_tasks.empty(); }))
          return false;
        task = std::move(m_tasks.front());
        m_tasks.pop_front();
      }
      task();
    }
    return true;
  }

private:
  mutex m_mutex;
  condition_variable m_condition;
  deque<function<void()>> m_tasks;
};

UNIT_CLASS_TEST(SearchAPITest, AddressResolutionCancellationAndInteractivePriority)
{
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-1, -1, 1, 1), 16);
  bool cancelledDelivered = false;
  bool foregroundDelivered = false;
  bool backgroundDelivered = false;
  api.ResolveAddress(1, {{"123 Main Street", "123 Main Street"}}, "en", true, [&](auto) { cancelledDelivered = true; });
  api.CancelAddressResolution(1);
  api.ResolveAddress(2, {{"123 Main Street", "123 Main Street"}}, "en", true,
                     [&](auto) { backgroundDelivered = true; });

  EverywhereSearchParams params;
  params.m_query = "cafe";
  params.m_inputLocale = "en";
  params.m_onResults = [](Results) {};
  api.SearchEverywhere(std::move(params));
  // An unsupported address selected while ordinary search is in flight must finish safely,
  // not borrow that search's results or index an empty query list.
  api.ResolveAddress(3, {}, "en", false, [&](auto result)
  {
    TEST(!result, ());
    TEST(!backgroundDelivered, ());
    foregroundDelivered = true;
  });
  TEST(delegate.WaitUntil([&] { return foregroundDelivered && backgroundDelivered; }), ());
  TEST(!cancelledDelivered, ());
  TEST(api.GetLastSearchQueries().empty(), ());
}

UNIT_CLASS_TEST(SearchAPITest, BackgroundAddressResolutionRequiresVisibleZoom)
{
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-1, -1, 1, 1), 15);
  bool backgroundDelivered = false;
  bool foregroundDelivered = false;
  api.ResolveAddress(1, {{"123 Main Street", "123 Main Street"}}, "en", true,
                     [&](auto) { backgroundDelivered = true; });
  api.ResolveAddress(2, {}, "en", false, [&](auto) { foregroundDelivered = true; });
  TEST(foregroundDelivered, ());
  TEST(!backgroundDelivered, ());
  api.OnViewportChanged(m2::RectD(-1, -1, 1, 1), 16);
  TEST(delegate.WaitUntil([&] { return backgroundDelivered; }), ());
}

UNIT_CLASS_TEST(SearchAPITest, AddressResolutionFindsMappedBuildingInBothModes)
{
  TestStreet street({m2::PointD(-0.001, 0), m2::PointD(0.001, 0)}, "Main Street", "en");
  TestBuilding building(m2::PointD(0, 0.00001), "", "123", "Main Street", "en");
  TestBuilding neighbour(m2::PointD(0.0002, 0.00004), "", "127", "Main Street", "en");
  BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(building);
    builder.Add(neighbour);
  });
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-0.002, -0.002, 0.002, 0.002), 16);
  for (bool background : {false, true})
  {
    bool delivered = false;
    api.ResolveAddress(background ? 2 : 1, {{"123 Main Street", "123 Main Street"}}, "en", background, [&](auto result)
    {
      TEST(result, (background));
      if (result)
      {
        TEST_ALMOST_EQUAL_ABS(result->GetFeatureCenter().x, building.GetCenter().x, 3e-6, ());
        TEST_ALMOST_EQUAL_ABS(result->GetFeatureCenter().y, building.GetCenter().y, 3e-6, ());
        auto qualified = *result;
        qualified.SetAddress("Sampletown, Canada");
        TEST(IsEarlyAddressResultMatchingQuery("123 Main Street Sampletown Canada", qualified, "123 Main Street"), ());
        TEST(!IsEarlyAddressResultMatchingQuery("123 Main Street", qualified, "123 Main Street"), ());
        TEST(!IsEarlyAddressResultMatchingQuery("123 Main Street West Sampletown Canada", qualified,
                                                "123 Main Street West"),
             ());
        TEST(!IsEarlyAddressResultMatchingQuery("123 Main Street Othertown Canada", qualified, "123 Main Street"), ());
      }
      delivered = true;
    });
    TEST(delegate.WaitUntil([&] { return delivered; }), (background));
    delivered = false;
    api.ResolveAddress(background ? 4 : 3, {{"125 Main Street", "125 Main Street"}}, "en", background, [&](auto result)
    {
      TEST(!result, (background));
      delivered = true;
    });
    TEST(delegate.WaitUntil([&] { return delivered; }), (background));
  }
}

UNIT_CLASS_TEST(SearchAPITest, TypedAddressRankingIsExplicitAndNeverInventsMissingAddresses)
{
  TestCity city(m2::PointD(0, 0), "Surrey", "en", 100);
  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });
  TestStreet street({m2::PointD(-0.001, 0), m2::PointD(0.001, 0)}, "131A Street", "en");
  TestBuilding first(m2::PointD(0, -0.00005), "", "6486", "131A Street", "en");
  TestBuilding second(m2::PointD(0.0002, -0.00005), "", "6492", "131A Street", "en");
  BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(city);
    builder.Add(street);
    builder.Add(first);
    builder.Add(second);
  });
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-0.002, -0.002, 0.002, 0.002), 16);
  for (bool enabled : {false, true})
    for (bool exact : {false, true})
    {
      bool delivered = false;
      EverywhereSearchParams params;
      params.m_query = exact ? "6492 131a st, surrey " : "6498 131a st, surrey ";
      params.m_inputLocale = "en";
      params.m_prioritizeAddressMatches = enabled;
      params.m_onResults = [&](Results results)
      {
        if (!results.IsEndMarker())
          return;
        TEST_GREATER(results.GetCount(), 0, (enabled, exact));
        if (enabled && results.GetCount() != 0)
        {
          TEST(IsAddressResultMatchingQuery("6492 131a st Surrey", results[0]), ());
          TEST_EQUAL(results[0].GetString().find("6498"), std::string::npos, ());
        }
        delivered = true;
      };
      TEST(api.SearchEverywhere(std::move(params)), ());
      TEST(delegate.WaitUntil([&] { return delivered; }), (enabled, exact));
    }
  for (auto const * number : {"18446744073709551615", "18446744073709551616", "999999999999999999999999999999"})
  {
    bool delivered = false;
    std::string const query = std::string(number) + " 131A Street Surrey";
    api.ResolveAddress(10, {{query, std::string(number) + " 131A Street"}}, "en", false, [&](auto result)
    {
      TEST(!result, (number));
      delivered = true;
    });
    TEST(delegate.WaitUntil([&] { return delivered; }), (number));
  }
}

UNIT_CLASS_TEST(SearchAPITest, AddressSearchFindsDistantStreetBeforeNearbyFuzzyMatches)
{
  BuildCountry("Nearland", [&](TestMwmBuilder & builder)
  {
    for (size_t i = 0; i < 120; ++i)
    {
      double const x = static_cast<double>(i) * 0.00001;
      TestStreet street({m2::PointD(x, 0), m2::PointD(x, 0.0002)}, "191A Street", "en");
      builder.Add(street);
    }
  });
  TestStreet street({m2::PointD(30, 0), m2::PointD(30.001, 0)}, "131A Street", "en");
  TestBuilding building(m2::PointD(30.0002, -0.00005), "", "6492", "131A Street", "en");
  BuildCountry("Farland", [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(building);
  });
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-0.002, -0.002, 0.002, 0.002), 16);
  for (auto const * query : {"6492 131a st", "6492 131a st ", "6498 131a st", "6498 131a st "})
  {
    bool delivered = false;
    EverywhereSearchParams params;
    params.m_query = query;
    params.m_inputLocale = "en";
    params.m_prioritizeAddressMatches = true;
    params.m_onResults = [&](Results results)
    {
      if (!results.IsEndMarker())
        return;
      TEST_EQUAL(results.GetSuggestsCount(), 0, (query));
      TEST_GREATER(results.GetCount(), 0, (query));
      if (results.GetCount() != 0)
      {
        TEST(IsAddressResultMatchingQuery("6492 131a st", results[0]), (query, results[0].GetString()));
        TEST_GREATER(mercator::DistanceOnEarth(results[0].GetFeatureCenter(), m2::PointD(0, 0)), 1000000, ());
      }
      delivered = true;
    };
    TEST(api.SearchEverywhere(std::move(params)), ());
    TEST(delegate.WaitUntil([&] { return delivered; }), (query));
  }
}

UNIT_CLASS_TEST(SearchAPITest, AddressResolutionPrefersMappedAddressOverInterpolation)
{
  auto const countryId = BuildCountry("Addressland", [&](TestMwmBuilder & builder)
  { builder.Add(TestBuilding(m2::PointD(0, 0), "", "123", "Main Street", "en")); });
  FeatureID const id(countryId, 0);
  auto const buildingType = classif().GetTypeByPath({"building"});
  auto const interpolationType = classif().GetTypeByPath({"addr:interpolation", "odd"});
  Results results;
  Result interpolated(m2::PointD(0, 0), "123, Main Street");
  interpolated.FromFeature(id, interpolationType, interpolationType, {});
  TEST_EQUAL(GetAddressResultMatch("123 Main Street", interpolated), AddressResultMatch::Interpolated, ());
  results.AddResultNoChecks(std::move(interpolated));
  Result mapped(m2::PointD(0.0001, 0), "123, Main Street");
  mapped.FromFeature(id, buildingType, buildingType, {});
  results.AddResultNoChecks(std::move(mapped));
  auto const * resolved = FindUniqueAddressResult("123 Main Street", results);
  TEST(resolved, ());
  TEST_EQUAL(resolved->GetFeatureCenter(), results[1].GetFeatureCenter(), ());
}

UNIT_CLASS_TEST(SearchAPITest, AddressSearchPreservesAmbiguityAndStreamsExactResults)
{
  for (double const x : {0.0, 30.0})
  {
    TestStreet street({m2::PointD(x - 0.001, 0), m2::PointD(x + 0.001, 0)}, "Main Street", "en");
    TestBuilding building(m2::PointD(x, 0.00001), "", "123", "Main Street", "en");
    BuildCountry(x == 0 ? "Nearland" : "Farland", [&](TestMwmBuilder & builder)
    {
      builder.Add(street);
      builder.Add(building);
    });
  }
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-0.002, -0.002, 0.002, 0.002), 16);
  bool finished = false;
  bool streamed = false;
  EverywhereSearchParams params;
  params.m_query = "123 Main Street";
  params.m_inputLocale = "en";
  params.m_prioritizeAddressMatches = true;
  params.m_onResults = [&](Results results)
  {
    if (!results.IsEndMarker())
    {
      for (auto const & result : results)
      {
        TEST(IsAddressResultMatchingQuery("123 Main Street", result), (result.GetString()));
        streamed = true;
      }
      return;
    }
    TEST_EQUAL(results.GetCount(), 2, ());
    TEST(!FindUniqueAddressResult("123 Main Street", results), ());
    finished = true;
  };
  TEST(api.SearchEverywhere(std::move(params)), ());
  TEST(delegate.WaitUntil([&] { return finished; }), ());
  TEST(streamed, ());
  bool resolved = false;
  api.ResolveAddress(1, {{"123 Main Street", "123 Main Street"}}, "en", false, [&](auto result)
  {
    TEST(!result, ());
    resolved = true;
  });
  TEST(delegate.WaitUntil([&] { return resolved; }), ());
}

UNIT_CLASS_TEST(SearchAPITest, AddressResolutionDoesNotDeliverAfterCompletionOrCancellation)
{
  TestStreet street({m2::PointD(-0.001, 0), m2::PointD(0.001, 0)}, "Main Street", "en");
  TestBuilding building(m2::PointD(0, 0.00001), "", "123", "Main Street", "en");
  BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(street);
    builder.Add(building);
  });
  QueuedDelegate delegate;
  SearchAPI api(m_dataSource, m_storage, *m_infoGetter, 1, delegate);
  api.OnViewportChanged(m2::RectD(-0.002, -0.002, 0.002, 0.002), 16);
  int delivered = 0;
  bool cancelledDelivered = false;
  api.ResolveAddress(1, {{"123 Main Street", "123 Main Street"}}, "en", false, [&](auto result)
  {
    TEST(result, ());
    ++delivered;
  });
  TEST(delegate.WaitUntil([&] { return delivered != 0; }), ());
  api.ResolveAddress(2, {{"123 Main Street", "123 Main Street"}}, "en", false,
                     [&](auto) { cancelledDelivered = true; });
  api.CancelAddressResolution(2);
  bool searchFinished = false;
  EverywhereSearchParams params;
  params.m_query = "Main Street";
  params.m_inputLocale = "en";
  params.m_onResults = [&](Results results)
  {
    if (results.IsEndMarker())
      delegate.RunUITask([&] { searchFinished = true; });
  };
  api.SearchEverywhere(std::move(params));
  TEST(delegate.WaitUntil([&] { return searchFinished; }), ());
  TEST_EQUAL(delivered, 1, ());
  TEST(!cancelledDelivered, ());
}

UNIT_CLASS_TEST(SearchAPITest, MultipleViewportsRequests)
{
  TestCafe cafe1(m2::PointD(0, 0), "cafe 1", "en");
  TestCafe cafe2(m2::PointD(0.5, 0.5), "cafe 2", "en");
  TestCafe cafe3(m2::PointD(10, 10), "cafe 3", "en");
  TestCafe cafe4(m2::PointD(10.5, 10.5), "cafe 4", "en");

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(cafe1);
    builder.Add(cafe2);
    builder.Add(cafe3);
    builder.Add(cafe4);
  });

  atomic<int> stage{0};

  promise<void> promise0;
  auto future0 = promise0.get_future();

  promise<void> promise1;
  auto future1 = promise1.get_future();

  ViewportSearchParams params;
  params.m_query = "cafe ";
  params.m_inputLocale = "en";

  params.m_onCompleted = [&](Results const & results)
  {
    TEST(!results.IsEndedCancelled(), ());

    if (!results.IsEndMarker())
      return;

    if (stage == 0)
    {
      Rules const rules = {ExactMatch(id, cafe1), ExactMatch(id, cafe2)};
      TEST(MatchResults(m_dataSource, rules, results), ());

      promise0.set_value();
    }
    else
    {
      TEST_EQUAL(stage, 1, ());
      Rules const rules = {ExactMatch(id, cafe3), ExactMatch(id, cafe4)};
      TEST(MatchResults(m_dataSource, rules, results), ());

      promise1.set_value();
    }
  };

  m_api.OnViewportChanged(m2::RectD(-1, -1, 1, 1));
  m_api.SearchInViewport(params);
  future0.wait();

  ++stage;
  m_api.OnViewportChanged(m2::RectD(9, 9, 11, 11));
  future1.wait();
}

UNIT_CLASS_TEST(SearchAPITest, ViewportPastAntimeridian)
{
  TestCafe cafe1(m2::PointD(0, 0), "cafe 1", "en");
  TestCafe cafe2(m2::PointD(0.5, 0.5), "cafe 2", "en");
  TestCafe cafe3(m2::PointD(10, 10), "cafe 3", "en");

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(cafe1);
    builder.Add(cafe2);
    builder.Add(cafe3);
  });

  promise<void> promise;
  auto future = promise.get_future();

  ViewportSearchParams params;
  params.m_query = "cafe ";
  params.m_inputLocale = "en";
  params.m_onCompleted = [&](Results const & results)
  {
    TEST(!results.IsEndedCancelled(), ());
    if (!results.IsEndMarker())
      return;

    Rules const rules = {ExactMatch(id, cafe1), ExactMatch(id, cafe2)};
    TEST(MatchResults(m_dataSource, rules, results), ());
    promise.set_value();
  };

  // The map scrolled past the antimeridian reports the viewport shifted by the whole world.
  m_api.OnViewportChanged(m2::RectD(359, -1, 361, 1));
  m_api.SearchInViewport(params);
  future.wait();
}

UNIT_CLASS_TEST(SearchAPITest, Cancellation)
{
  TestCafe cafe(m2::PointD(0, 0), "cafe", "en");

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  EverywhereSearchParams commonParams;
  commonParams.m_query = "cafe ";
  commonParams.m_inputLocale = "en";

  {
    auto params = commonParams;

    promise<void> promise;
    auto future = promise.get_future();

    params.m_onResults = [&](Results const & results)
    {
      TEST(!results.IsEndedCancelled(), ());

      if (!results.IsEndMarker())
        return;

      Rules const rules = {ExactMatch(id, cafe)};
      TEST(MatchResults(m_dataSource, rules, results), ());

      promise.set_value();
    };

    m_api.OnViewportChanged(m2::RectD(0.0, 0.0, 1.0, 1.0));
    m_api.SearchEverywhere(params);
    future.wait();
  }

  {
    auto params = commonParams;

    promise<void> promise;
    auto future = promise.get_future();

    params.m_timeout = chrono::seconds(-1);

    params.m_onResults = [&](Results const & results)
    {
      // The deadline has fired but Search API does not expose it.
      TEST(!results.IsEndedCancelled(), ());

      if (!results.IsEndMarker())
        return;

      Rules const rules = {ExactMatch(id, cafe)};
      TEST(MatchResults(m_dataSource, rules, results), ());

      promise.set_value();
    };

    // Force the search by changing the viewport.
    m_api.OnViewportChanged(m2::RectD(0.0, 0.0, 2.0, 2.0));
    m_api.SearchEverywhere(params);
    future.wait();
  }
}

UNIT_CLASS_TEST(SearchAPITest, BookmarksSearch)
{
  // BookmarkInfo stores a non-owning pointer, so each entry needs its own BookmarkData.
  vector<kml::BookmarkData> data(3);
  kml::SetDefaultStr(data[0].m_name, "R&R dinner");
  kml::SetDefaultStr(data[0].m_description, "They've got a cherry pie there that'll kill ya!");
  kml::SetDefaultStr(data[1].m_name, "Silver Mustang Casino");
  kml::SetDefaultStr(data[1].m_description, "Joyful place, owners Bradley and Rodney are very friendly!");
  kml::SetDefaultStr(data[2].m_name, "Great Northern Hotel");
  kml::SetDefaultStr(data[2].m_description, "Clean place with a reasonable price");

  vector<BookmarkInfo> marks;
  for (kml::MarkId i = 0; i < data.size(); ++i)
    marks.emplace_back(i, &data[i]);
  m_api.EnableIndexingOfBookmarksDescriptions(true);
  m_api.EnableIndexingOfBookmarkGroup(10, true /* enable */);
  m_api.OnBookmarksCreated(marks);
  m_api.OnViewportChanged(m2::RectD(-1, -1, 1, 1));

  auto runTest = [&](string const & query, kml::MarkGroupId const & groupId, vector<kml::MarkId> const & expected)
  {
    promise<vector<kml::MarkId>> idsPromise;
    auto idsFuture = idsPromise.get_future();

    BookmarksSearchParams params;
    params.m_query = query;
    params.m_onResults = [&](vector<kml::MarkId> const & results, BookmarksSearchParams::Status status)
    {
      if (status != BookmarksSearchParams::Status::Completed)
        return;
      idsPromise.set_value(results);
    };
    params.m_groupId = groupId;

    m_api.SearchInBookmarks(params);

    auto const ids = idsFuture.get();
    TEST_EQUAL(ids, expected, ());
  };

  string const query = "gread silver hotel";
  runTest(query, kml::kInvalidMarkGroupId, vector<kml::MarkId>());

  {
    vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(10), vector<kml::MarkId>({0, 1}));
    groupInfos.emplace_back(kml::MarkGroupId(11), vector<kml::MarkId>({2}));
    m_api.OnBookmarksAttached(groupInfos);
  }

  runTest(query, kml::kInvalidMarkGroupId, vector<kml::MarkId>({1}));
  runTest(query, kml::MarkGroupId(11), {});
  m_api.EnableIndexingOfBookmarkGroup(11, true /* enable */);
  runTest(query, kml::kInvalidMarkGroupId, vector<kml::MarkId>({2, 1}));
  runTest(query, kml::MarkGroupId(11), vector<kml::MarkId>({2}));
  m_api.EnableIndexingOfBookmarkGroup(11, false /* enable */);
  runTest(query, kml::kInvalidMarkGroupId, vector<kml::MarkId>({1}));
  runTest(query, kml::MarkGroupId(11), {});
  m_api.EnableIndexingOfBookmarkGroup(11, true /* enable */);

  {
    vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(10), vector<kml::MarkId>({1}));
    m_api.OnBookmarksDetached(groupInfos);
  }
  {
    vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(11), vector<kml::MarkId>({1}));
    m_api.OnBookmarksAttached(groupInfos);
  }
  runTest(query, kml::MarkGroupId(11), vector<kml::MarkId>({2, 1}));

  m_api.ResetBookmarksEngine();
  runTest(query, kml::MarkGroupId(11), {});
}
}  // namespace
}  // namespace search_api_tests

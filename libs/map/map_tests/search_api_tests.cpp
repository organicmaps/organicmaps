// NOTE: the purpose of this test is to test interaction between
// SearchAPI and search engine. If you would like to test search
// engine behaviour, please, consider implementing another search
// integration test.

#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "map/bookmarks_search_params.hpp"
#include "map/search_api.hpp"
#include "map/search_product_info.hpp"
#include "map/viewport_search_params.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "indexer/classificator.hpp"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>

namespace
{
using Rules = std::vector<std::shared_ptr<search::tests_support::MatchingRule>>;

struct TestCafe : public generator::tests_support::TestPOI
{
public:
  TestCafe(m2::PointD const & center, std::string const & name, std::string const & lang) : TestPOI(center, name, lang)
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
  void RunUITask(std::function<void()> fn) override { fn(); }
};

class SearchAPITest : public generator::tests_support::TestWithCustomMwms
{
public:
  SearchAPITest()
    : m_infoGetter(storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform()))
    , m_api(m_dataSource, m_storage, *m_infoGetter, 1 /* numThreads */, m_delegate)
  {}

protected:
  storage::Storage m_storage;
  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  Delegate m_delegate;
  SearchAPI m_api;
};

UNIT_CLASS_TEST(SearchAPITest, MultipleViewportsRequests)
{
  TestCafe cafe1(m2::PointD(0, 0), "cafe 1", "en");
  TestCafe cafe2(m2::PointD(0.5, 0.5), "cafe 2", "en");
  TestCafe cafe3(m2::PointD(10, 10), "cafe 3", "en");
  TestCafe cafe4(m2::PointD(10.5, 10.5), "cafe 4", "en");

  auto const id = BuildCountry("Wonderland", [&](generator::tests_support::TestMwmBuilder & builder)
  {
    builder.Add(cafe1);
    builder.Add(cafe2);
    builder.Add(cafe3);
    builder.Add(cafe4);
  });

  std::atomic<int> stage{0};

  std::promise<void> promise0;
  auto future0 = promise0.get_future();

  std::promise<void> promise1;
  auto future1 = promise1.get_future();

  search::ViewportSearchParams params;
  params.m_query = "cafe ";
  params.m_inputLocale = "en";

  params.m_onCompleted = [&](search::Results const & results)
  {
    TEST(!results.IsEndedCancelled(), ());

    if (!results.IsEndMarker())
      return;

    if (stage == 0)
    {
      Rules const rules = {search::tests_support::ExactMatch(id, cafe1), search::tests_support::ExactMatch(id, cafe2)};
      TEST(search::tests_support::MatchResults(m_dataSource, rules, results), ());

      promise0.set_value();
    }
    else
    {
      TEST_EQUAL(stage, 1, ());
      Rules const rules = {search::tests_support::ExactMatch(id, cafe3), search::tests_support::ExactMatch(id, cafe4)};
      TEST(search::tests_support::MatchResults(m_dataSource, rules, results), ());

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

UNIT_CLASS_TEST(SearchAPITest, Cancellation)
{
  TestCafe cafe(m2::PointD(0, 0), "cafe", "en");

  auto const id =
      BuildCountry("Wonderland", [&](generator::tests_support::TestMwmBuilder & builder) { builder.Add(cafe); });

  search::EverywhereSearchParams commonParams;
  commonParams.m_query = "cafe ";
  commonParams.m_inputLocale = "en";

  {
    auto params = commonParams;

    std::promise<void> promise;
    auto future = promise.get_future();

    params.m_onResults = [&](search::Results const & results, std::vector<search::ProductInfo> const &)
    {
      TEST(!results.IsEndedCancelled(), ());

      if (!results.IsEndMarker())
        return;

      Rules const rules = {search::tests_support::ExactMatch(id, cafe)};
      TEST(search::tests_support::MatchResults(m_dataSource, rules, results), ());

      promise.set_value();
    };

    m_api.OnViewportChanged(m2::RectD(0.0, 0.0, 1.0, 1.0));
    m_api.SearchEverywhere(params);
    future.wait();
  }

  {
    auto params = commonParams;

    std::promise<void> promise;
    auto future = promise.get_future();

    params.m_timeout = std::chrono::seconds(-1);

    params.m_onResults = [&](search::Results const & results, std::vector<search::ProductInfo> const &)
    {
      // The deadline has fired but Search API does not expose it.
      TEST(!results.IsEndedCancelled(), ());

      if (!results.IsEndMarker())
        return;

      Rules const rules = {search::tests_support::ExactMatch(id, cafe)};
      TEST(search::tests_support::MatchResults(m_dataSource, rules, results), ());

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
  std::vector<BookmarkInfo> marks;

  kml::BookmarkData data;
  kml::SetDefaultStr(data.m_name, "R&R dinner");
  kml::SetDefaultStr(data.m_description, "They've got a cherry pie there that'll kill ya!");
  marks.emplace_back(0, data);
  kml::SetDefaultStr(data.m_name, "Silver Mustang Casino");
  kml::SetDefaultStr(data.m_description, "Joyful place, owners Bradley and Rodney are very friendly!");
  marks.emplace_back(1, data);
  kml::SetDefaultStr(data.m_name, "Great Northern Hotel");
  kml::SetDefaultStr(data.m_description, "Clean place with a reasonable price");
  marks.emplace_back(2, data);
  m_api.EnableIndexingOfBookmarksDescriptions(true);
  m_api.EnableIndexingOfBookmarkGroup(10, true /* enable */);
  m_api.OnBookmarksCreated(marks);
  m_api.OnViewportChanged(m2::RectD(-1, -1, 1, 1));

  auto runTest =
      [&](std::string const & query, kml::MarkGroupId const & groupId, std::vector<kml::MarkId> const & expected)
  {
    std::promise<std::vector<kml::MarkId>> idsPromise;
    auto idsFuture = idsPromise.get_future();

    search::BookmarksSearchParams params;
    params.m_query = query;
    params.m_onResults = [&](std::vector<kml::MarkId> const & results, search::BookmarksSearchParams::Status status)
    {
      if (status != search::BookmarksSearchParams::Status::Completed)
        return;
      idsPromise.set_value(results);
    };
    params.m_groupId = groupId;

    m_api.SearchInBookmarks(params);

    auto const ids = idsFuture.get();
    TEST_EQUAL(ids, expected, ());
  };

  std::string const query = "gread silver hotel";
  runTest(query, kml::kInvalidMarkGroupId, std::vector<kml::MarkId>());

  {
    std::vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(10), std::vector<kml::MarkId>({0, 1}));
    groupInfos.emplace_back(kml::MarkGroupId(11), std::vector<kml::MarkId>({2}));
    m_api.OnBookmarksAttached(groupInfos);
  }

  runTest(query, kml::kInvalidMarkGroupId, std::vector<kml::MarkId>({1}));
  runTest(query, kml::MarkGroupId(11), {});
  m_api.EnableIndexingOfBookmarkGroup(11, true /* enable */);
  runTest(query, kml::kInvalidMarkGroupId, std::vector<kml::MarkId>({2, 1}));
  runTest(query, kml::MarkGroupId(11), std::vector<kml::MarkId>({2}));
  m_api.EnableIndexingOfBookmarkGroup(11, false /* enable */);
  runTest(query, kml::kInvalidMarkGroupId, std::vector<kml::MarkId>({1}));
  runTest(query, kml::MarkGroupId(11), {});
  m_api.EnableIndexingOfBookmarkGroup(11, true /* enable */);

  {
    std::vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(10), std::vector<kml::MarkId>({1}));
    m_api.OnBookmarksDetached(groupInfos);
  }
  {
    std::vector<BookmarkGroupInfo> groupInfos;
    groupInfos.emplace_back(kml::MarkGroupId(11), std::vector<kml::MarkId>({1}));
    m_api.OnBookmarksAttached(groupInfos);
  }
  runTest(query, kml::MarkGroupId(11), std::vector<kml::MarkId>({2, 1}));

  m_api.ResetBookmarksEngine();
  runTest(query, kml::MarkGroupId(11), {});
}
}  // namespace

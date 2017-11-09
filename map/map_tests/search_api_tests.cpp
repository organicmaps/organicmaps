// NOTE: the purpose of this test is to test interaction between
// SearchAPI and search engine. If you would like to test search
// engine behaviour, please, consider implementing another search
// integration test.

#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "map/search_api.hpp"
#include "map/viewport_search_params.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "indexer/classificator.hpp"
#include "indexer/index.hpp"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>

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
  TestCafe(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
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

class SearchAPITest : public TestWithCustomMwms
{
public:
  SearchAPITest()
    : m_infoGetter(CountryInfoReader::CreateCountryInfoReader(GetPlatform()))
    , m_api(m_index, m_storage, *m_infoGetter, m_delegate)
  {
  }

protected:
  Storage m_storage;
  unique_ptr<CountryInfoGetter> m_infoGetter;
  Delegate m_delegate;
  SearchAPI m_api;
};

UNIT_CLASS_TEST(SearchAPITest, MultipleViewportsRequests)
{
  TestCafe cafe1(m2::PointD(0, 0), "cafe 1", "en");
  TestCafe cafe2(m2::PointD(0, 0), "cafe 2", "en");
  TestCafe cafe3(m2::PointD(10, 10), "cafe 3", "en");
  TestCafe cafe4(m2::PointD(10, 10), "cafe 4", "en");

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) {
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

  params.m_onCompleted = [&](Results const & results) {
    TEST(!results.IsEndedCancelled(), ());

    if (!results.IsEndMarker())
      return;

    if (stage == 0)
    {
      Rules const rules = {ExactMatch(id, cafe1), ExactMatch(id, cafe2)};
      TEST(MatchResults(m_index, rules, results), ());

      promise0.set_value();
    }
    else
    {
      TEST_EQUAL(stage, 1, ());
      Rules const rules = {ExactMatch(id, cafe3), ExactMatch(id, cafe4)};
      TEST(MatchResults(m_index, rules, results), ());

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
}  // namespace

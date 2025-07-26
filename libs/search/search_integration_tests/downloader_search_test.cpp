#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "search/downloader_search_callback.hpp"
#include "search/mode.hpp"
#include "search/result.hpp"
#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "storage/downloader_queue_universal.hpp"
#include "storage/downloader_search_params.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

#include "platform/downloader_defines.hpp"

#include "geometry/rect2d.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace std;

class DataSource;

namespace search
{
namespace
{
string const kCountriesTxt = R"({
  "id": "Countries",
  "v": )" + strings::to_string(0 /* version */) +
                             R"(,
  "g": [
      {
       "id": "Flatland",
       "g": [
        {
         "id": "Square One",
         "s": 123,
         "old": [
          "Flatland"
         ]
        },
        {
         "id": "Square Two",
         "s": 456,
         "old": [
          "Flatland"
         ]
        }
       ]
      },
      {
       "id": "Wonderland",
       "g": [
        {
         "id": "Shortpondville",
         "s": 789,
         "old": [
          "Wonderland"
         ]
        },
        {
         "id": "Longpondville",
         "s": 100,
         "old": [
          "Wonderland"
         ]
        }
       ]
      }
   ]})";

class TestMapFilesDownloader : public storage::MapFilesDownloader
{
public:
  // MapFilesDownloader overrides:
  void Remove(storage::CountryId const &) override {}
  void Clear() override {}

  storage::QueueInterface const & GetQueue() const override { return m_queue; }

private:
  void GetMetaConfig(MetaConfigCallback const &) override {}
  void Download(storage::QueuedCountry &&) override {}

  storage::Queue m_queue;
};

class TestDelegate : public DownloaderSearchCallback::Delegate
{
public:
  // DownloaderSearchCallback::Delegate overrides:
  void RunUITask(function<void()> fn) override { fn(); }
};

class DownloaderSearchRequest
  : public TestSearchRequest
  , public TestDelegate
{
public:
  DownloaderSearchRequest(DataSource & dataSource, TestSearchEngine & engine, string const & query)
    : TestSearchRequest(engine, MakeSearchParams(query))
    , m_storage(kCountriesTxt, make_unique<TestMapFilesDownloader>())
    , m_downloaderCallback(static_cast<DownloaderSearchCallback::Delegate &>(*this), dataSource,
                           m_engine.GetCountryInfoGetter(), m_storage, MakeDownloaderParams(query))
  {
    SetCustomOnResults(bind(&DownloaderSearchRequest::OnResultsDownloader, this, placeholders::_1));
  }

  void OnResultsDownloader(search::Results const & results)
  {
    m_downloaderCallback(results);
    OnResults(results);
  }

  vector<storage::DownloaderSearchResult> const & GetResults() const { return m_downloaderResults; }

private:
  search::SearchParams MakeSearchParams(string const & query)
  {
    search::SearchParams p;
    p.m_query = query;
    p.m_inputLocale = "en";
    p.m_viewport = m2::RectD(0, 0, 1, 1);
    p.m_mode = search::Mode::Downloader;
    p.m_suggestsEnabled = false;
    return p;
  }

  storage::DownloaderSearchParams MakeDownloaderParams(string const & query)
  {
    storage::DownloaderSearchParams p;
    p.m_query = query;
    p.m_inputLocale = "en";
    p.m_onResults = [this](storage::DownloaderSearchResults const & r)
    {
      CHECK(!m_endMarker, ());

      auto const & results = r.m_results;
      CHECK_GREATER_OR_EQUAL(results.size(), m_downloaderResults.size(), ());
      CHECK(equal(m_downloaderResults.begin(), m_downloaderResults.end(), results.begin()), ());

      m_downloaderResults = r.m_results;
      if (r.m_endMarker)
        m_endMarker = true;
    };
    return p;
  }

  vector<storage::DownloaderSearchResult> m_downloaderResults;
  bool m_endMarker = false;

  storage::Storage m_storage;

  DownloaderSearchCallback m_downloaderCallback;
};

class DownloaderSearchTest : public SearchTest
{
public:
  void AddRegion(string const & countryName, string const & regionName, m2::PointD const & p1, m2::PointD const & p2)
  {
    TestPOI cornerPost1(p1, regionName + " corner post 1", "en");
    TestPOI cornerPost2(p2, regionName + " corner post 2", "en");
    TestCity capital((p1 + p2) * 0.5, regionName + " capital", "en", 0 /* rank */);
    TestCountry country(p1 * 0.3 + p2 * 0.7, countryName, "en");
    BuildCountry(regionName, [&](TestMwmBuilder & builder)
    {
      builder.Add(cornerPost1);
      builder.Add(cornerPost2);
      builder.Add(capital);
      if (!countryName.empty())
      {
        // Add the country feature to one region only.
        builder.Add(country);
        m_worldCountries.push_back(country);
      }
    });
    m_worldCities.push_back(capital);
  }

  void BuildWorld()
  {
    SearchTest::BuildWorld([&](TestMwmBuilder & builder)
    {
      for (auto const & ft : m_worldCountries)
        builder.Add(ft);
      for (auto const & ft : m_worldCities)
        builder.Add(ft);
    });
  }

private:
  vector<TestCountry> m_worldCountries;
  vector<TestCity> m_worldCities;
};

template <typename T>
void TestResults(vector<T> received, vector<T> expected)
{
  sort(received.begin(), received.end());
  sort(expected.begin(), expected.end());
  TEST_EQUAL(expected, received, ());
}

UNIT_CLASS_TEST(DownloaderSearchTest, Smoke)
{
  AddRegion("Flatland", "Squareland One", m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0));
  AddRegion("", "Squareland Two", m2::PointD(1.0, 1.0), m2::PointD(3.0, 3.0));
  AddRegion("Wonderland", "Shortpondland", m2::PointD(-1.0, -1.0), m2::PointD(0.0, 0.0));
  AddRegion("", "Longpondland", m2::PointD(-3.0, -3.0), m2::PointD(-1.0, -1.0));
  BuildWorld();

  {
    DownloaderSearchRequest request(m_dataSource, m_engine, "squareland one");
    request.Run();

    TestResults(request.GetResults(), {storage::DownloaderSearchResult("Squareland One", "Squareland One capital")});
  }

  {
    DownloaderSearchRequest request(m_dataSource, m_engine, "shortpondland");
    request.Run();

    TestResults(request.GetResults(), {storage::DownloaderSearchResult("Shortpondland", "Shortpondland capital")});
  }

  {
    DownloaderSearchRequest request(m_dataSource, m_engine, "flatland");
    request.Run();

    TestResults(request.GetResults(), {storage::DownloaderSearchResult("Flatland", "Flatland")});
  }

  {
    DownloaderSearchRequest request(m_dataSource, m_engine, "squareland");
    request.Run();

    TestResults(request.GetResults(), {storage::DownloaderSearchResult("Squareland One", "Squareland One capital"),
                                       storage::DownloaderSearchResult("Squareland Two", "Squareland Two capital")});
  }
}
}  // namespace
}  // namespace search

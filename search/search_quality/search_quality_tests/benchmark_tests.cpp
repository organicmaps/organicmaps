#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "platform/local_country_file_utils.hpp"

namespace benchmark_tests
{
using namespace search;
using namespace std::chrono;

class DataSourceTestFixture : public generator::tests_support::TestWithClassificator
{
protected:
  FrozenDataSource m_dataSource;
  std::unique_ptr<storage::CountryInfoGetterForTesting> m_countryInfo;

public:
  DataSourceTestFixture()
    : m_countryInfo(std::make_unique<storage::CountryInfoGetterForTesting>())
  {
    using namespace platform;
    std::vector<LocalCountryFile> localFiles;
    FindAllLocalMapsAndCleanup(std::numeric_limits<int64_t>::max() /* latestVersion */, localFiles);

    bool hasWorld = false;
    for (auto const & file : localFiles)
    {
      auto const res = m_dataSource.RegisterMap(file);
      TEST_EQUAL(res.second, MwmSet::RegResult::Success, ());

      auto const & info = res.first.GetInfo();
      TEST_EQUAL(file.GetCountryName(), info->GetCountryName(), ());
      if (file.GetCountryName() == WORLD_FILE_NAME)
      {
        TEST_EQUAL(info->GetType(), MwmInfo::WORLD, ());
        hasWorld = true;
      }

      m_countryInfo->AddCountry({ info->GetCountryName(), info->m_bordersRect });
    }

    TEST(hasWorld, ());
  }
};

class SearchBenchmarkFixture : public DataSourceTestFixture
{
  tests_support::TestSearchEngine m_engine;
  m2::RectD m_viewport;

public:
  SearchBenchmarkFixture() : m_engine(m_dataSource, std::move(m_countryInfo), {})
  {
  }

  void SetCenter(ms::LatLon ll)
  {
    m_viewport = mercator::MetersToXY(ll.m_lon, ll.m_lat, 1.0E4);
  }

  uint64_t Run(std::string const & query)
  {
    tests_support::TestSearchRequest request(m_engine, query, "en", Mode::Everywhere, m_viewport);
    request.Run();
    return request.ResponseTime().count();
  }
};

UNIT_CLASS_TEST(SearchBenchmarkFixture, Smoke)
{
  SetCenter({ 50.1052, 8.6868 }); // Frankfurt am Main
  LOG(LINFO, (Run("b")));
}
} // namespace benchmark_tests

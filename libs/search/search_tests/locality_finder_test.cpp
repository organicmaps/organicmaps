#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"

#include "search/categories_cache.hpp"
#include "search/locality_finder.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/cancellable.hpp"

#include <string>
#include <vector>

namespace locality_finder_test
{
class LocalityFinderTest : public generator::tests_support::TestWithClassificator
{
  platform::LocalCountryFile m_worldFile;

  FrozenDataSource m_dataSource;

  base::Cancellable m_cancellable;
  search::VillagesCache m_villagesCache;
  search::CitiesBoundariesTable m_boundariesTable;

  search::LocalityFinder m_finder;
  m2::RectD m_worldRect;

public:
  LocalityFinderTest()
    : m_villagesCache(m_cancellable)
    , m_boundariesTable(m_dataSource)
    , m_finder(m_dataSource, m_boundariesTable, m_villagesCache)
  {
    m_worldFile = platform::LocalCountryFile::MakeForTesting("World");

    try
    {
      auto const p = m_dataSource.Register(m_worldFile);
      TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());

      MwmSet::MwmId const & id = p.first;
      TEST(id.IsAlive(), ());

      m_worldRect = id.GetInfo()->m_bordersRect;
      m_boundariesTable.Load();
    }
    catch (RootException const & ex)
    {
      LOG(LERROR, ("Read World.mwm error:", ex.Msg()));
    }
  }

  ~LocalityFinderTest() { platform::CountryIndexes::DeleteFromDisk(m_worldFile); }

  void RunTests(std::vector<ms::LatLon> const & input, char const * results[])
  {
    for (size_t i = 0; i < input.size(); ++i)
    {
      std::string_view result;
      m_finder.GetLocality(mercator::FromLatLon(input[i]), [&](search::LocalityItem const & item)
      { item.GetSpecifiedOrDefaultName(StringUtf8Multilang::kEnglishCode, result); });
      TEST_EQUAL(result, results[i], ());
    }
  }

  m2::RectD const & GetWorldRect() const { return m_worldRect; }

  void ClearCaches() { m_finder.ClearCache(); }
};

UNIT_CLASS_TEST(LocalityFinderTest, Smoke)
{
  std::vector<ms::LatLon> input;
  input.emplace_back(53.8993094, 27.5433964);  // Minsk
  input.emplace_back(48.856517, 2.3521);       // Paris
  input.emplace_back(52.5193859, 13.3908289);  // Berlin

  char const * results[] = {"Minsk", "Paris", "Berlin"};

  RunTests(input, results);

  ClearCaches();
  input.clear();

  input.emplace_back(41.875, -87.624367);      // Chicago
  input.emplace_back(-22.911225, -43.209384);  // Rio de Janeiro
  input.emplace_back(-37.8142, 144.96);        // Melbourne (Australia)
  input.emplace_back(53.883931, 27.69341);     // Parking Minsk (near MKAD)
  input.emplace_back(53.917306, 27.707875);    // Lipki airport (Minsk)
  input.emplace_back(42.285901, 18.834407);    // Budva (Montenegro)
  input.emplace_back(43.9363996, 12.4466991);  // City of San Marino
  input.emplace_back(47.3345002, 8.531262);    // Zurich

  char const * results3[] = {"Chicago", "Rio de Janeiro", "Melbourne",          "Minsk",
                             "Minsk",   "Budva",          "City of San Marino", "Zurich"};

  RunTests(input, results3);
}

UNIT_CLASS_TEST(LocalityFinderTest, Moscow)
{
  std::vector<ms::LatLon> input;
  input.emplace_back(55.80166, 37.54066);  // Krasnoarmeyskaya 30

  char const * results[] = {"Moscow"};

  RunTests(input, results);
}
}  // namespace locality_finder_test

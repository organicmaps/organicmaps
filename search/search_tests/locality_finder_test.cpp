#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/index.hpp"
#include "indexer/classificator_loader.hpp"

#include "search/locality_finder.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"


namespace
{

class LocalityFinderTest
{
  platform::LocalCountryFile m_worldFile;
  Index m_index;
  search::LocalityFinder m_finder;
  m2::RectD m_worldRect;

public:
  LocalityFinderTest() : m_finder(&m_index)
  {
    classificator::Load();
    m_worldFile = platform::LocalCountryFile::MakeForTesting("World");

    try
    {
      auto const p = m_index.Register(m_worldFile);
      TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());

      MwmSet::MwmId const & id = p.first;
      TEST(id.IsAlive(), ());

      m_worldRect = id.GetInfo()->m_limitRect;
    }
    catch (RootException const & ex)
    {
      LOG(LERROR, ("Read World.mwm error:", ex.Msg()));
    }

    m_finder.SetLanguage(StringUtf8Multilang::GetLangIndex("en"));
  }

  ~LocalityFinderTest()
  {
    platform::CountryIndexes::DeleteFromDisk(m_worldFile);
  }

  void RunTestsViewport(vector<ms::LatLon> const & input, char const * results[])
  {
    for (size_t i = 0; i < input.size(); ++i)
    {
      string result;
      m_finder.GetLocalityInViewport(MercatorBounds::FromLatLon(input[i]), result);
      TEST_EQUAL(result, results[i], ());
    }
  }

  void RunTestsEverywhere(vector<ms::LatLon> const & input, char const * results[])
  {
    for (size_t i = 0; i < input.size(); ++i)
    {
      string result;
      m_finder.GetLocalityCreateCache(MercatorBounds::FromLatLon(input[i]), result);
      TEST_EQUAL(result, results[i], ());
    }
  }

  m2::RectD const & GetWorldRect() const { return m_worldRect; }
  void SetViewportByIndex(m2::RectD const & viewport, size_t ind)
  {
    m_finder.SetViewportByIndex(viewport, ind);
  }
  void ClearCaches() { m_finder.ClearCacheAll(); }
};

} // namespace

/*
UNIT_CLASS_TEST(LocalityFinderTest, Smoke)
{
  m2::RectD const & rect = GetWorldRect();
  SetViewportByIndex(rect, 0);

  vector<ms::LatLon> input;
  input.emplace_back(53.8993094, 27.5433964);   // Minsk
  input.emplace_back(48.856517, 2.3521);        // Paris
  input.emplace_back(52.5193859, 13.3908289);   // Berlin

  char const * results[] =
  {
    "Minsk",
    "Paris",
    "Berlin"
  };

  // Tets one viewport based on whole map
  RunTestsViewport(input, results);

  // Test two viewport based on quaters of world map
  m2::RectD rect1;
  rect1.setMinX(rect.minX());
  rect1.setMinY(rect.minY());
  rect1.setMaxX(rect.Center().x);
  rect1.setMaxY(rect.Center().y);

  m2::RectD rect2;
  rect2.setMinX(rect.Center().x);
  rect2.setMinY(rect.Center().y);
  rect2.setMaxY(rect.maxY());
  rect2.setMaxX(rect.maxX());

  input.clear();
  input.emplace_back(41.875, -87.624367);      // Chicago
  input.emplace_back(-22.911225, -43.209384);  // Rio de Janeiro
  input.emplace_back(-37.8142, 144.96);        // Melbourne (Australia)
  input.emplace_back(53.883931, 27.69341);     // Parkin Minsk (near MKAD)
  input.emplace_back(53.917306, 27.707875);    // Lipki airport (Minsk)
  input.emplace_back(42.285901, 18.834407);    // Budva (Montenegro)
  input.emplace_back(41.903479, 12.452854);    // Vaticano (Rome)
  input.emplace_back(47.3345002, 8.531262);    // Zurich

  SetViewportByIndex(rect1, 0);
  SetViewportByIndex(rect2, 1);

  char const * results2[] =
  {
    "",
    "Rio de Janeiro",
    "",
    "Minsk",
    "Minsk",
    "Budva",
    "Rome",
    "Zurich"
  };

  RunTestsViewport(input, results2);

  ClearCaches();

  char const * results3[] =
  {
    "Chicago",
    "Rio de Janeiro",
    "Melbourne",
    "Minsk",
    "Minsk",
    "Budva",
    "Rome",
    "Zurich"
  };

  RunTestsEverywhere(input, results3);
}
*/

UNIT_CLASS_TEST(LocalityFinderTest, Moscow)
{
  vector<ms::LatLon> input;
  input.emplace_back(55.80166, 37.54066);   // Krasnoarmeyskaya 30

  char const * results[] =
  {
    "Moscow"
  };

  RunTestsEverywhere(input, results);
}

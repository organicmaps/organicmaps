#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/index.hpp"
#include "indexer/classificator_loader.hpp"

#include "search/locality_finder.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

namespace
{

void doTests(search::LocalityFinder & finder, vector<m2::PointD> const & input, char const * results[])
{
  for (size_t i = 0; i < input.size(); ++i)
  {
    string result;
    finder.GetLocalityInViewport(MercatorBounds::FromLatLon(input[i].y, input[i].x), result);
    TEST_EQUAL(result, results[i], ());
  }
}


void doTests2(search::LocalityFinder & finder, vector<m2::PointD> const & input, char const * results[])
{
  for (size_t i = 0; i < input.size(); ++i)
  {
    string result;
    finder.GetLocalityCreateCache(MercatorBounds::FromLatLon(input[i].y, input[i].x), result);
    TEST_EQUAL(result, results[i], ());
  }
}

}

UNIT_TEST(LocalityFinder)
{
  classificator::Load();

  Index index;
  m2::RectD rect;

  auto world = platform::LocalCountryFile::MakeForTesting("World");
  auto cleanup = [&world]()
  {
    platform::CountryIndexes::DeleteFromDisk(world);
  };
  MY_SCOPE_GUARD(cleanupOnExit, cleanup);
  cleanup();

  try
  {
    auto const p = index.Register(world);
    TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());

    MwmSet::MwmId const & id = p.first;
    TEST(id.IsAlive(), ());

    rect = id.GetInfo()->m_limitRect;
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Read World.mwm error:", ex.Msg()));
  }

  search::LocalityFinder finder(&index);
  finder.SetLanguage(StringUtf8Multilang::GetLangIndex("en"));
  finder.SetViewportByIndex(MercatorBounds::FullRect(), 0);

  vector<m2::PointD> input;
  input.push_back(m2::PointD(27.5433964, 53.8993094)); // Minsk
  input.push_back(m2::PointD(2.3521, 48.856517)); // Paris
  input.push_back(m2::PointD(13.3908289, 52.5193859)); // Berlin

  char const * results[] =
  {
    "Minsk",
    "Paris",
    "Berlin"
  };

  // Tets one viewport based on whole map
  doTests(finder, input, results);

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
  input.push_back(m2::PointD(-87.624367, 41.875));  // Chicago
  input.push_back(m2::PointD(-43.209384, -22.911225));  // Rio de Janeiro
  input.push_back(m2::PointD(144.96, -37.8142)); // Melbourne (Australia)
  input.push_back(m2::PointD(27.69341, 53.883931)); // Parkin Minsk (near MKAD)
  input.push_back(m2::PointD(27.707875, 53.917306)); // Lipki airport (Minsk)
  input.push_back(m2::PointD(18.834407, 42.285901)); // Budva (Montenegro)
  input.push_back(m2::PointD(12.452854, 41.903479)); // Vaticano (Rome)
  input.push_back(m2::PointD(8.531262, 47.3345002)); // Zurich

  finder.SetViewportByIndex(rect1, 0);
  finder.SetViewportByIndex(rect2, 1);

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

  doTests(finder, input, results2);

  finder.ClearCacheAll();

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

  doTests2(finder, input, results3);
}

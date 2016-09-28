#include "testing/testing.hpp"

#include "search/locality_finder.hpp"

using namespace search;

namespace
{
string GetMatchedCity(m2::PointD const & point, vector<LocalityFinder::Item> const & items)
{
  string name;
  LocalitySelector selector(name, point);
  for (auto const & item : items)
    selector(item);
  return name;
}

// TODO (@y): this test fails for now. Need to uncomment it as soon as
// locality finder will be fixed.
//
// UNIT_TEST(LocalitySelector_Test1)
// {
//   auto const name = GetMatchedCity(
//       m2::PointD(-97.563458662952925238, 26.796728721236661386),
//       {{"Matamoros", m2::PointD(-97.506656349498797454, 26.797180986068354969), 10000},

//        {"Brownsville", m2::PointD(-97.489103971612919963, 26.845589500139880101), 180663}});
//   TEST_EQUAL(name, "Matamoros", ());
// }

UNIT_TEST(LocalitySelector_Test2)
{
  vector<LocalityFinder::Item> const localities = {
      {"Moscow", m2::PointD(37.617513849438893203, 67.45398133444564337), 11971516},
      {"Krasnogorsk", m2::PointD(37.340409438658895169, 67.58036703829372982), 135735},
      {"Khimki", m2::PointD(37.444994145035053634, 67.700701677882875629), 240463},
      {"Mytishchi", m2::PointD(37.733943192236807818, 67.736750571340394345), 180663},
      {"Dolgoprudny", m2::PointD(37.514259518892714595, 67.780738804428438016), 101979}};

  {
    auto const name =
        GetMatchedCity(m2::PointD(37.538269143836714647, 67.535546478148901883), localities);
    TEST_EQUAL(name, "Moscow", ());
  }

  {
    auto const name =
        GetMatchedCity(m2::PointD(37.469807099326018829, 67.666502652067720192), localities);
    TEST_EQUAL(name, "Khimki", ());
  }
}
}  // namespace

#include "testing/testing.hpp"

#include "search/locality_finder.hpp"

using namespace search;

namespace
{
string GetMatchedCity(m2::PointD const & point, vector<LocalityItem> const & items)
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
//       m2::PointD(-97.56345, 26.79672),
//       {{"Matamoros", m2::PointD(-97.50665, 26.79718), 10000},

//        {"Brownsville", m2::PointD(-97.48910, 26.84558), 180663}});
//   TEST_EQUAL(name, "Matamoros", ());
// }

UNIT_TEST(LocalitySelector_Test2)
{
  vector<LocalityItem> const localities = {
      {"Moscow", m2::PointD(37.61751, 67.45398), 11971516},
      {"Krasnogorsk", m2::PointD(37.34040, 67.58036), 135735},
      {"Khimki", m2::PointD(37.44499, 67.70070), 240463},
      {"Mytishchi", m2::PointD(37.73394, 67.73675), 180663},
      {"Dolgoprudny", m2::PointD(37.51425, 67.78073), 101979}};

  {
    auto const name =
        GetMatchedCity(m2::PointD(37.53826, 67.53554), localities);
    TEST_EQUAL(name, "Moscow", ());
  }

  {
    auto const name =
        GetMatchedCity(m2::PointD(37.46980, 67.66650), localities);
    TEST_EQUAL(name, "Khimki", ());
  }
}
}  // namespace

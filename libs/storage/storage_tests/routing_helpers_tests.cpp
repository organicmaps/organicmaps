#include "testing/testing.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "routing_common/num_mwm_tree.hpp"

#include "coding/point_coding.hpp"

#include <algorithm>
#include <vector>

namespace storage_routing_helpers_tests
{
UNIT_TEST(NumMwmTree_WrappedBounds)
{
  std::vector<storage::CountryDef> const countries = {{"East", {170, -10, 190, 10}},
                                                      {"West", {-190, 20, -170, 30}},
                                                      {"Offset", {184, 40, 193, 50}},
                                                      {"Canonical", {-10, -10, 10, 10}},
                                                      {"Touch", {10, -10, 20, 10}},
                                                      {"AllLongitudes", {-180.000001, -50, 180.000001, 60}},
                                                      {"ShiftedWorld", {-540, -50, -180, 60}},
                                                      {"Zero", {0, 0, 1, 1}},
                                                      {"EastTouch", {170, 70, 180, 80}},
                                                      {"WestTouch", {-180, 70, -170, 80}},
                                                      {"EastPoint", {180, 90, 180, 90}},
                                                      {"WestPoint", {-180, 90, -180, 90}}};
  storage::CountryInfoGetterForTesting getter(countries);
  routing::NumMwmIds ids;
  for (auto const & country : countries)
    ids.RegisterFile(platform::CountryFile(country.m_countryId));
  auto const tree = routing::MakeNumMwmTree(ids, getter);
  for (auto const & query :
       {m2::RectD(-175, 0, -175, 0),   m2::RectD(185, 0, 185, 0),      m2::RectD(545, 0, 545, 0),
        m2::RectD(-535, 0, -535, 0),   m2::RectD(175, 25, 175, 25),    m2::RectD(-175, 25, -175, 25),
        m2::RectD(-170, 45, -170, 45), m2::RectD(170, -10, 190, 30),   m2::RectD(-190, -10, -170, 30),
        m2::RectD(180, -10, 180, 30),  m2::RectD(-180, -10, -180, 30), m2::RectD(10, 0, 10, 0),
        m2::RectD(-180, -50, 180, 60), m2::RectD(-720, -60, 720, 70),  m2::RectD(0, 61, 0, 61),
        m2::RectD(0, 0, 0, 0),         m2::RectD(-180, 70, -180, 80),  m2::RectD(180, 70, 180, 80),
        m2::RectD(-185, 75, -180, 75), m2::RectD(180, 75, 185, 75),    m2::RectD(-180, 90, -180, 90),
        m2::RectD(180, 90, 180, 90),   m2::RectD(0, -50, 360, 60),     m2::RectD(-360, -50, 0, 60),
        m2::RectD(100, -50, 500, 60),  m2::RectD(-500, -50, -100, 60)})
  {
    auto const expectedQuery = mercator::WrapRectX(m2::Inflate(query, kMwmPointAccuracy, kMwmPointAccuracy));
    std::vector<routing::NumMwmId> expected;
    for (routing::NumMwmId id = 0; id < countries.size(); ++id)
    {
      storage::CountryDef const country(countries[id].m_countryId, mercator::WrapRectX(countries[id].m_rect));
      if (country.IsRectOverlap(expectedQuery))
        expected.push_back(id);
    }
    std::vector<routing::NumMwmId> actual;
    routing::ForEachNumMwmInRect(*tree, query, [&](routing::NumMwmId id) { actual.push_back(id); });
    std::sort(actual.begin(), actual.end());
    TEST_EQUAL(actual, expected, (query));
  }
}

UNIT_TEST(NumMwmTree_QueryInflation)
{
  m4::Tree<routing::NumMwmId> tree;
  tree.Add(0, {10, 10, 20, 20});
  tree.Add(1, {-180, 10, -170, 20});
  tree.Add(2, {170, 10, 180, 20});

  auto const test = [&](m2::PointD const & point, std::vector<routing::NumMwmId> const & expected)
  {
    std::vector<routing::NumMwmId> actual;
    routing::ForEachNumMwmInRect(tree, {point, point}, [&](routing::NumMwmId id) { actual.push_back(id); });
    std::sort(actual.begin(), actual.end());
    TEST_EQUAL(actual, expected, (point));
  };

  double constexpr kGap = kMwmPointAccuracy / 2;
  test({10 - kGap, 15}, {0});
  test({20 + kGap, 15}, {0});
  test({15, 10 - kGap}, {0});
  test({15, 20 + kGap}, {0});
  test({10 - kGap, 20 + kGap}, {0});
  test({180 - kGap, 15}, {1, 2});
  test({-180 + kGap, 15}, {1, 2});
  test({540 - kGap, 15}, {1, 2});
  test({-540 + kGap, 15}, {1, 2});

  test({10 - 2 * kMwmPointAccuracy, 15}, {});
  test({15, 20 + 2 * kMwmPointAccuracy}, {});
  test({180 - 2 * kMwmPointAccuracy, 15}, {2});
  test({-180 + 2 * kMwmPointAccuracy, 15}, {1});
}
}  // namespace storage_routing_helpers_tests

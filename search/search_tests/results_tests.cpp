#include "testing/testing.hpp"

#include "search/result.hpp"

#include "indexer/data_source.hpp"

#include <iterator>

namespace search_tests
{

UNIT_TEST(Results_Sorting)
{
  FrozenDataSource dataSource;
  MwmSet::MwmId const id = dataSource.Register(platform::LocalCountryFile::MakeForTesting("minsk-pass")).first;

  search::Results r;
  for (uint32_t i = 5; i != 0; --i)
  {
    search::Result res(m2::PointD::Zero(), {});
    res.FromFeature({id, i}, 0, 0, {});
    r.AddResultNoChecks(std::move(res));
  }

  for (auto it = r.begin(); it != r.end(); ++it)
  {
    auto const & result = *it;
    TEST_EQUAL(result.GetFeatureID().m_index, std::distance(it, r.end()), ());
    TEST_EQUAL(result.GetPositionInResults(), std::distance(r.begin(), it), ());
  }

  r.SortBy([](auto const & lhs, auto const & rhs) { return lhs.GetFeatureID().m_index < rhs.GetFeatureID().m_index; });

  for (auto it = r.begin(); it != r.end(); ++it)
  {
    auto const & result = *it;
    TEST_EQUAL(result.GetFeatureID().m_index, std::distance(r.begin(), it) + 1, ());
    TEST_EQUAL(result.GetPositionInResults(), std::distance(r.begin(), it), ());
  }
}

UNIT_TEST(Result_PrependCity)
{
  {
    search::Result r(m2::PointD::Zero(), {});
    r.SetAddress("Moscow, Russia");
    r.PrependCity("Moscow");
    TEST_EQUAL(r.GetAddress(), "Moscow, Russia", ());
  }

  {
    search::Result r(m2::PointD::Zero(), "улица Михася Лынькова");
    r.SetAddress("Минская область, Беларусь");
    r.PrependCity("Минск");
    TEST_EQUAL(r.GetAddress(), "Минск, Минская область, Беларусь", ());
  }
}
}  // namespace search_tests

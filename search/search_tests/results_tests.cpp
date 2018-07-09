#include "testing/testing.hpp"

#include "search/result.hpp"

#include <iterator>

namespace search
{
UNIT_TEST(Results_Sorting)
{
  Results r;
  MwmSet::MwmId id;
  for (uint32_t i = 5; i != 0; --i)
  {
    r.AddResultNoChecks({{id, i}, {} /* pt */, {} /* str */, {} /* address */,
                         {} /* featureTypeName */, {} /* featureType */, {} /* metadata */});
  }

  for (auto it = r.begin(); it != r.end(); ++it)
  {
    auto const & result = *it;
    TEST_EQUAL(result.GetFeatureID().m_index, std::distance(it, r.end()), ());
    TEST_EQUAL(result.GetPositionInResults(), std::distance(r.begin(), it), ());
  }

  r.SortBy([](auto const & lhs, auto const & rhs) {
    return lhs.GetFeatureID().m_index < rhs.GetFeatureID().m_index;
  });

  for (auto it = r.begin(); it != r.end(); ++it)
  {
    auto const & result = *it;
    TEST_EQUAL(result.GetFeatureID().m_index, std::distance(r.begin(), it) + 1, ());
    TEST_EQUAL(result.GetPositionInResults(), std::distance(r.begin(), it), ());
  }
}

UNIT_TEST(Result_PrependCity)
{
  FeatureID const fid;
  Result::Metadata const meta;

  {
    Result r(fid, m2::PointD::Zero(), "" /* str */, "Moscow, Russia" /* address */,
             "" /* featureTypeName */, 0 /* featureType */, meta);

    r.PrependCity("Moscow");
    TEST_EQUAL(r.GetAddress(), "Moscow, Russia", ());
  }

  {
    Result r(fid, m2::PointD::Zero(), "улица Михася Лынькова" /* str */,
             "Минская область, Беларусь" /* address */, "" /* featureTypeName */,
             0 /* featureType */, meta);

    r.PrependCity("Минск");
    TEST_EQUAL(r.GetAddress(), "Минск, Минская область, Беларусь", ());
  }
}
}  // namespace search

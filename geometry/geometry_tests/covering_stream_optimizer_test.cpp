#include "../../testing/testing.hpp"
#include "../cellid.hpp"
#include "../covering.hpp"
#include "../covering_stream_optimizer.hpp"
#include "../../base/macros.hpp"

typedef m2::CellId<5> CellId;

namespace
{
  struct TestSink
  {
    vector<pair<int64_t, int> > & m_Res;

    TestSink(vector<pair<int64_t, int> > & res) : m_Res(res) {}

    void operator() (int64_t cell, int value) const
    {
      m_Res.push_back(make_pair(cell, value));
    }
  };
}

UNIT_TEST(CoveringStreamOptimizer_Smoke)
{
  // my::g_LogLevel = LDEBUG;
  vector<pair<int64_t, int> > res1;
  TestSink sink(res1);
  covering::CoveringStreamOptimizer<CellId, int, TestSink> optimizer(sink, 5, 4);
  optimizer.Add(CellId("01").ToInt64(), 1);
  optimizer.Add(CellId("01").ToInt64(), 2);
  optimizer.Add(CellId("0200").ToInt64(), 1);
  optimizer.Add(CellId("0201").ToInt64(), 1);
  optimizer.Add(CellId("0202").ToInt64(), 1);
  optimizer.Add(CellId("021").ToInt64(), 1);
  optimizer.Add(CellId("022").ToInt64(), 1);
  optimizer.Flush();
  vector<pair<CellId, int> > e, res;
  for (size_t i = 0; i < res1.size(); ++i)
    res.push_back(make_pair(CellId::FromInt64(res1[i].first), res1[i].second));
  e.push_back(make_pair(CellId("01"), 1));
  e.push_back(make_pair(CellId("01"), 2));
  e.push_back(make_pair(CellId("02"), 1));
  TEST_EQUAL(e, res, ());
}

UNIT_TEST(CoveringStreamOptimizer_MinId1)
{
  int64_t const minId = 72200890973LL;
  int64_t ids[] = { 72200890973, 72200890995, 72200891005, 72200891011, 72200891013,
                    72200891014, 72200891015, 72200891036, 72200896894, 72200896902 };
  vector<int64_t> idsV(&ids[0], &ids[0] + ARRAY_SIZE(ids));
  covering::CoveringStreamOptimizer<m2::CellId<19>, int, TestSink>::Optimize(idsV, minId);
}

UNIT_TEST(CoveringStreamOptimizer_MinId2)
{
  int64_t const minId = 72200890973LL;
  int64_t ids[] = { 72200890973, 72200890994, 72200891015, 72200891036};
  vector<int64_t> idsV(&ids[0], &ids[0] + ARRAY_SIZE(ids));
  covering::CoveringStreamOptimizer<m2::CellId<19>, int, TestSink>::Optimize(idsV, minId);
}


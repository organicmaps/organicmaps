#include "testing/testing.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/indexer_tests/bounds.hpp"

#include "coding/hex.hpp"

#include <cmath>
#include <random>
#include <string>
#include <utility>

using namespace std;

typedef m2::CellId<30> CellIdT;

UNIT_TEST(ToCellId)
{
  string s("2130000");
  s.append(CellIdT::DEPTH_LEVELS - 1 - s.size(), '0');
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::ToCellId(1.5, 2.5).ToString()), s, ());
  TEST_EQUAL(CellIdT::FromString(s), (CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::ToCellId(1.5, 2.5)), ());
}

UNIT_TEST(CommonCell)
{
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::Cover2PointsWithCell(3.5, 2.5, 2.5, 3.5)),
             CellIdT::FromString("3"), ());
  TEST_EQUAL((CellIdConverter<Bounds<0, 0, 4, 4>, CellIdT>::Cover2PointsWithCell(2.25, 1.75, 2.75, 1.25)),
             CellIdT::FromString("12"), ());
}

namespace
{
template <typename T1, typename T2>
bool PairsAlmostEqualULPs(pair<T1, T1> const & p1, pair<T2, T2> const & p2)
{
  return fabs(p1.first - p2.first) + fabs(p1.second - p2.second) < 0.00001;
}
}  // namespace

UNIT_TEST(CellId_RandomRecode)
{
  mt19937 rng(0);
  for (size_t i = 0; i < 1000; ++i)
  {
    uint32_t const x = rng() % 2000;
    uint32_t const y = rng() % 1000;
    m2::PointD const pt = CellIdConverter<Bounds<0, 0, 2000, 1000>, CellIdT>::FromCellId(
        CellIdConverter<Bounds<0, 0, 2000, 1000>, CellIdT>::ToCellId(x, y));
    TEST(fabs(pt.x - x) < 0.0002, (x, y, pt));
    TEST(fabs(pt.y - y) < 0.0001, (x, y, pt));
  }
}

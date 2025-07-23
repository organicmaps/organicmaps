#include "testing/testing.hpp"

#include "geometry/cellid.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace cellid_test
{
using std::make_pair, std::string, std::vector;

UNIT_TEST(CellId_Parent)
{
  TEST_EQUAL(m2::CellId<3>("1").Parent(), m2::CellId<3>(""), ());
  TEST_EQUAL(m2::CellId<4>("032").Parent(), m2::CellId<4>("03"), ());
}

UNIT_TEST(CellId_AncestorAtLevel)
{
  TEST_EQUAL(m2::CellId<3>("1").AncestorAtLevel(0), m2::CellId<3>(""), ());
  TEST_EQUAL(m2::CellId<4>("032").AncestorAtLevel(2), m2::CellId<4>("03"), ());
  TEST_EQUAL(m2::CellId<4>("032").AncestorAtLevel(1), m2::CellId<4>("0"), ());
  TEST_EQUAL(m2::CellId<4>("032").AncestorAtLevel(0), m2::CellId<4>(""), ());
}

UNIT_TEST(CellId_FromString)
{
  TEST_EQUAL(m2::CellId<3>(""), m2::CellId<3>::FromBitsAndLevel(0, 0), ());
  TEST_EQUAL(m2::CellId<4>("032"), m2::CellId<4>::FromBitsAndLevel(14, 3), ());
  TEST_EQUAL(m2::CellId<3>("03"), m2::CellId<3>::FromBitsAndLevel(3, 2), ());
}

UNIT_TEST(CellId_ToString)
{
  TEST_EQUAL(m2::CellId<3>("").ToString(), "", ());
  TEST_EQUAL(m2::CellId<4>("032").ToString(), "032", ());
  TEST_EQUAL(m2::CellId<3>("03").ToString(), "03", ());
}

UNIT_TEST(CellId_ToInt64)
{
  TEST_EQUAL(m2::CellId<3>("").ToInt64(3), 1, ());
  TEST_EQUAL(m2::CellId<3>("0").ToInt64(3), 2, ());
  TEST_EQUAL(m2::CellId<3>("1").ToInt64(3), 7, ());
  TEST_EQUAL(m2::CellId<3>("2").ToInt64(3), 12, ());
  TEST_EQUAL(m2::CellId<3>("3").ToInt64(3), 17, ());
  TEST_EQUAL(m2::CellId<3>("00").ToInt64(3), 3, ());
  TEST_EQUAL(m2::CellId<3>("01").ToInt64(3), 4, ());
  TEST_EQUAL(m2::CellId<3>("03").ToInt64(3), 6, ());
  TEST_EQUAL(m2::CellId<3>("10").ToInt64(3), 8, ());
  TEST_EQUAL(m2::CellId<3>("20").ToInt64(3), 13, ());
  TEST_EQUAL(m2::CellId<3>("23").ToInt64(3), 16, ());
  TEST_EQUAL(m2::CellId<3>("30").ToInt64(3), 18, ());
  TEST_EQUAL(m2::CellId<3>("31").ToInt64(3), 19, ());
  TEST_EQUAL(m2::CellId<3>("33").ToInt64(3), 21, ());
}

UNIT_TEST(CellId_ToInt64_LevelLessThanDepth)
{
  TEST_EQUAL(m2::CellId<3>("").ToInt64(2), 1, ());
  TEST_EQUAL(m2::CellId<3>("0").ToInt64(2), 2, ());
  TEST_EQUAL(m2::CellId<3>("1").ToInt64(2), 3, ());
  TEST_EQUAL(m2::CellId<3>("2").ToInt64(2), 4, ());
  TEST_EQUAL(m2::CellId<3>("3").ToInt64(2), 5, ());
  TEST_EQUAL(m2::CellId<3>("00").ToInt64(2), 2, ());
  TEST_EQUAL(m2::CellId<3>("01").ToInt64(2), 2, ());
  TEST_EQUAL(m2::CellId<3>("03").ToInt64(2), 2, ());
  TEST_EQUAL(m2::CellId<3>("10").ToInt64(2), 3, ());
  TEST_EQUAL(m2::CellId<3>("20").ToInt64(2), 4, ());
  TEST_EQUAL(m2::CellId<3>("23").ToInt64(2), 4, ());
  TEST_EQUAL(m2::CellId<3>("30").ToInt64(2), 5, ());
  TEST_EQUAL(m2::CellId<3>("31").ToInt64(2), 5, ());
  TEST_EQUAL(m2::CellId<3>("33").ToInt64(2), 5, ());
}

UNIT_TEST(CellId_FromInt64)
{
  TEST_EQUAL(m2::CellId<3>(""), m2::CellId<3>::FromInt64(1, 3), ());
  TEST_EQUAL(m2::CellId<3>("0"), m2::CellId<3>::FromInt64(2, 3), ());
  TEST_EQUAL(m2::CellId<3>("1"), m2::CellId<3>::FromInt64(7, 3), ());
  TEST_EQUAL(m2::CellId<3>("2"), m2::CellId<3>::FromInt64(12, 3), ());
  TEST_EQUAL(m2::CellId<3>("3"), m2::CellId<3>::FromInt64(17, 3), ());
  TEST_EQUAL(m2::CellId<3>("00"), m2::CellId<3>::FromInt64(3, 3), ());
  TEST_EQUAL(m2::CellId<3>("01"), m2::CellId<3>::FromInt64(4, 3), ());
  TEST_EQUAL(m2::CellId<3>("03"), m2::CellId<3>::FromInt64(6, 3), ());
  TEST_EQUAL(m2::CellId<3>("10"), m2::CellId<3>::FromInt64(8, 3), ());
  TEST_EQUAL(m2::CellId<3>("20"), m2::CellId<3>::FromInt64(13, 3), ());
  TEST_EQUAL(m2::CellId<3>("23"), m2::CellId<3>::FromInt64(16, 3), ());
  TEST_EQUAL(m2::CellId<3>("30"), m2::CellId<3>::FromInt64(18, 3), ());
  TEST_EQUAL(m2::CellId<3>("31"), m2::CellId<3>::FromInt64(19, 3), ());
  TEST_EQUAL(m2::CellId<3>("33"), m2::CellId<3>::FromInt64(21, 3), ());
}

UNIT_TEST(CellId_XY)
{
  TEST_EQUAL(m2::CellId<3>("").XY(), make_pair(4U, 4U), ());
  TEST_EQUAL(m2::CellId<3>("0").XY(), make_pair(2U, 2U), ());
  TEST_EQUAL(m2::CellId<3>("1").XY(), make_pair(6U, 2U), ());
  TEST_EQUAL(m2::CellId<3>("2").XY(), make_pair(2U, 6U), ());
  TEST_EQUAL(m2::CellId<3>("3").XY(), make_pair(6U, 6U), ());
  TEST_EQUAL(m2::CellId<3>("00").XY(), make_pair(1U, 1U), ());
  TEST_EQUAL(m2::CellId<3>("01").XY(), make_pair(3U, 1U), ());
  TEST_EQUAL(m2::CellId<3>("03").XY(), make_pair(3U, 3U), ());
  TEST_EQUAL(m2::CellId<3>("10").XY(), make_pair(5U, 1U), ());
  TEST_EQUAL(m2::CellId<3>("20").XY(), make_pair(1U, 5U), ());
  TEST_EQUAL(m2::CellId<3>("23").XY(), make_pair(3U, 7U), ());
  TEST_EQUAL(m2::CellId<3>("30").XY(), make_pair(5U, 5U), ());
  TEST_EQUAL(m2::CellId<3>("31").XY(), make_pair(7U, 5U), ());
  TEST_EQUAL(m2::CellId<3>("33").XY(), make_pair(7U, 7U), ());
  TEST_EQUAL(m2::CellId<3>("33").XY(), make_pair(7U, 7U), ());
}

UNIT_TEST(CellId_Radius)
{
  TEST_EQUAL(m2::CellId<3>("").Radius(), 4, ());
  TEST_EQUAL(m2::CellId<3>("1").Radius(), 2, ());
  TEST_EQUAL(m2::CellId<3>("00").Radius(), 1, ());
}

UNIT_TEST(CellId_FromXY)
{
  TEST_EQUAL((m2::CellId<3>::FromXY(0, 0, 2)), (m2::CellId<3>("00")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(0, 0, 1)), (m2::CellId<3>("0")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(0, 0, 0)), (m2::CellId<3>("")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(5, 4, 0)), (m2::CellId<3>("")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(5, 0, 2)), (m2::CellId<3>("10")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(5, 0, 1)), (m2::CellId<3>("1")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(5, 0, 1)), (m2::CellId<3>("1")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(7, 7, 2)), (m2::CellId<3>("33")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(7, 7, 1)), (m2::CellId<3>("3")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(7, 7, 0)), (m2::CellId<3>("")), ());
  TEST_EQUAL((m2::CellId<3>::FromXY(8, 8, 2)), (m2::CellId<3>("33")), ());
}

UNIT_TEST(CellId_FromXY_XY_Match)
{
  TEST_EQUAL((m2::CellId<9>::FromXY(48, 80, 8).XY()), make_pair(49U, 81U), ());
  TEST_EQUAL((m2::CellId<9>::FromXY(192, 320, 8).XY()), make_pair(193U, 321U), ());
  TEST_EQUAL((m2::CellId<11>::FromXY(768, 1280, 10).XY()), make_pair(769U, 1281U), ());
  TEST_EQUAL((m2::CellId<21>::FromXY(786432, 1310720, 20).XY()), make_pair(786433U, 1310721U), ());
}

UNIT_TEST(CellId_SubTreeSize)
{
  TEST_EQUAL(m2::CellId<3>("00").SubTreeSize(3), 1, ());
  TEST_EQUAL(m2::CellId<3>("22").SubTreeSize(3), 1, ());
  TEST_EQUAL(m2::CellId<3>("33").SubTreeSize(3), 1, ());
  TEST_EQUAL(m2::CellId<3>("0").SubTreeSize(3), 5, ());
  TEST_EQUAL(m2::CellId<3>("1").SubTreeSize(3), 5, ());
  TEST_EQUAL(m2::CellId<3>("3").SubTreeSize(3), 5, ());
  TEST_EQUAL(m2::CellId<3>("").SubTreeSize(3), 21, ());
}

UNIT_TEST(CellId_LessQueueOrder)
{
  vector<string> v;
  v.emplace_back("0");
  v.emplace_back("1");
  v.emplace_back("00");
  v.emplace_back("00");
  v.emplace_back("02");
  v.emplace_back("002");
  v.emplace_back("101");
  vector<string> e = v;
  do
  {
    vector<m2::CellId<4>> tst, exp;
    for (size_t i = 0; i < v.size(); ++i)
    {
      tst.emplace_back(e[i]);
      exp.emplace_back(v[i]);
    }
    sort(tst.begin(), tst.end(), m2::CellId<4>::LessLevelOrder());
    TEST_EQUAL(tst, exp, ());
  }
  while (next_permutation(e.begin(), e.end()));
}

UNIT_TEST(CellId_LessStackOrder)
{
  vector<string> v;
  v.emplace_back("0");
  v.emplace_back("00");
  v.emplace_back("00");
  v.emplace_back("002");
  v.emplace_back("02");
  v.emplace_back("1");
  v.emplace_back("101");
  vector<string> e = v;
  do
  {
    vector<m2::CellId<4>> tst, exp;
    for (size_t i = 0; i < v.size(); ++i)
    {
      tst.emplace_back(e[i]);
      exp.emplace_back(v[i]);
    }
    sort(tst.begin(), tst.end(), m2::CellId<4>::LessPreOrder());
    TEST_EQUAL(tst, exp, ());
  }
  while (next_permutation(e.begin(), e.end()));
}

UNIT_TEST(CellId_IsStringValid)
{
  using Id = m2::CellId<9>;
  TEST(Id::IsCellId("0123132"), ());
  TEST(Id::IsCellId(""), ());
  TEST(!Id::IsCellId("-1332"), ());
  TEST(!Id::IsCellId("023."), ());
  TEST(!Id::IsCellId("121832"), ());
}

UNIT_TEST(CellId_ToAndFromInt64ZOrder)
{
  int const kMaxDepth = 4;
  using Id = m2::CellId<kMaxDepth>;
  for (int depth = 1; depth <= kMaxDepth; ++depth)
  {
    int64_t const treeSize = ((int64_t{1} << (2 * depth)) - 1) / 3;
    LOG(LINFO, ("Depth =", depth, " TreeSize =", treeSize));
    for (int64_t id = 1; id <= treeSize; ++id)
    {
      auto const cell = Id::FromInt64ZOrder(id, depth);
      TEST_EQUAL(id, cell.ToInt64ZOrder(depth), ());
    }
  }

  vector<string> const atDepth3 = {
      "",  "0",  "00", "01", "02", "03", "1",  "10", "11", "12", "13",
      "2", "20", "21", "22", "23", "3",  "30", "31", "32", "33",
  };

  for (uint64_t id = 1; id <= atDepth3.size(); ++id)
    TEST_EQUAL(Id::FromInt64(id, 3), Id(atDepth3[id - 1]), ());
}
}  // namespace cellid_test

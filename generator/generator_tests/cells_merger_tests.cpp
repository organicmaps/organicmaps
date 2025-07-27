#include "testing/testing.hpp"

#include "generator/cells_merger.hpp"

#include <vector>

namespace
{
UNIT_TEST(CellsMerger_Empty)
{
  generator::cells_merger::CellsMerger merger({});
  std::vector<m2::RectD> expected;
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(CellsMerger_One)
{
  generator::cells_merger::CellsMerger merger({{{0.0, 0.0}, {1.0, 1.0}}});
  std::vector<m2::RectD> expected{{{0.0, 0.0}, {1.0, 1.0}}};
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(CellsMerger_Two)
{
  generator::cells_merger::CellsMerger merger({{{0.0, 0.0}, {1.0, 1.0}}, {{1.0, 0.0}, {2.0, 1.0}}});
  std::vector<m2::RectD> expected{{{0.0, 0.0}, {1.0, 1.0}}, {{1.0, 0.0}, {2.0, 1.0}}};
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(CellsMerger_Four)
{
  generator::cells_merger::CellsMerger merger(
      {{{0.0, 0.0}, {1.0, 1.0}}, {{1.0, 0.0}, {2.0, 1.0}}, {{0.0, 1.0}, {1.0, 2.0}}, {{1.0, 1.0}, {2.0, 2.0}}});
  std::vector<m2::RectD> expected{{{0.0, 0.0}, {2.0, 2.0}}};
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(CellsMerger_Six)
{
  generator::cells_merger::CellsMerger merger({{{0.0, 0.0}, {1.0, 1.0}},
                                               {{1.0, 0.0}, {2.0, 1.0}},
                                               {{0.0, 1.0}, {1.0, 2.0}},
                                               {{1.0, 1.0}, {2.0, 2.0}},
                                               {{2.0, 0.0}, {3.0, 1.0}},
                                               {{2.0, 1.0}, {3.0, 2.0}}});
  std::vector<m2::RectD> expected{{{1.0, 0.0}, {3.0, 2.0}}, {{0.0, 0.0}, {1.0, 1.0}}, {{0.0, 1.0}, {1.0, 2.0}}};
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(CellsMerger_Eight)
{
  generator::cells_merger::CellsMerger merger({{{0.0, 0.0}, {1.0, 1.0}},
                                               {{1.0, 0.0}, {2.0, 1.0}},
                                               {{0.0, 1.0}, {1.0, 2.0}},
                                               {{1.0, 1.0}, {2.0, 2.0}},
                                               {{2.0, 0.0}, {3.0, 1.0}},
                                               {{2.0, 1.0}, {3.0, 2.0}},
                                               {{3.0, 0.0}, {4.0, 1.0}},
                                               {{3.0, 1.0}, {4.0, 2.0}}});
  std::vector<m2::RectD> expected{{{1.0, 0.0}, {3.0, 2.0}},
                                  {{0.0, 0.0}, {1.0, 1.0}},
                                  {{0.0, 1.0}, {1.0, 2.0}},
                                  {{3.0, 0.0}, {4.0, 1.0}},
                                  {{3.0, 1.0}, {4.0, 2.0}}};
  auto const result = merger.Merge();
  TEST_EQUAL(result, expected, ());
}
}  // namespace

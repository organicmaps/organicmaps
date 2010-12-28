#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../tree4d.hpp"

namespace
{
  typedef m4::Tree<m2::RectD> tree_t;

  bool compare_true(m2::RectD const &, m2::RectD const &) { return true; }
}

UNIT_TEST(Tree4D_Smoke)
{
  tree_t theTree;

  m2::RectD arr[] = {
    m2::RectD(0, 0, 1, 1),
    m2::RectD(1, 1, 2, 2),
    m2::RectD(2, 2, 3, 3)
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    theTree.ReplaceIf(arr[i], arr[i], &compare_true);

  vector<m2::RectD> test;
  theTree.ForEach(MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  m2::RectD const replaceR(0.5, 0.5, 2.5, 2.5);
  theTree.ReplaceIf(replaceR, replaceR, &compare_true);

  test.clear();
  theTree.ForEach(MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], replaceR, ());
}

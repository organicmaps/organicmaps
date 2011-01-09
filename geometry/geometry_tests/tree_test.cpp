#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../tree4d.hpp"

namespace
{
  typedef m4::Tree<m2::RectD> tree_t;

  bool compare_true(m2::RectD const &, m2::RectD const &) { return true; }
  bool compare_false(m2::RectD const &, m2::RectD const &) { return false; }
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

UNIT_TEST(Tree4D_DrawTexts)
{
  tree_t theTree;

  m2::RectD arr[] = {
    m2::RectD(8, 13, 554, 32),
    m2::RectD(8, 33, 554, 52),
    m2::RectD(8, 54, 554, 73),
    m2::RectD(8, 76, 554, 95)
  };

  m2::RectD arr1[] = {
    m2::RectD(3, 23, 257, 42),
    m2::RectD(3, 43, 257, 62),
    m2::RectD(3, 65, 257, 84),
    m2::RectD(3, 87, 257, 106)
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    theTree.ReplaceIf(arr[i], arr[i], &compare_false);
    theTree.ReplaceIf(arr1[i], arr1[i], &compare_false);
  }

  vector<m2::RectD> test;
  theTree.ForEach(MakeBackInsertFunctor(test));
  TEST_EQUAL(4, test.size(), ());
  for (int i = 0; i < test.size(); ++i)
    TEST_EQUAL(test[i], arr[i], ());
}

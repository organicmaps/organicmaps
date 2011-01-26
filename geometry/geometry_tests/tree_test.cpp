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

  test.clear();
  m2::RectD const searchR(1.5, 1.5, 1.5, 1.5);
  theTree.ForEachInRect(searchR, MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], arr[1], ());

  m2::RectD const replaceR(0.5, 0.5, 2.5, 2.5);
  theTree.ReplaceIf(replaceR, replaceR, &compare_true);

  test.clear();
  theTree.ForEach(MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], replaceR, ());

  test.clear();
  theTree.ForEachInRect(searchR, MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
}

UNIT_TEST(Tree4D_ReplaceIf)
{
  tree_t theTree;

  m2::RectD arr[] = {
    m2::RectD(8, 13, 554, 32), m2::RectD(555, 13, 700, 32),
    m2::RectD(8, 33, 554, 52), m2::RectD(555, 33, 700, 52),
    m2::RectD(8, 54, 554, 73), m2::RectD(555, 54, 700, 73),
    m2::RectD(8, 76, 554, 95), m2::RectD(555, 76, 700, 95)
  };

  m2::RectD arr1[] = {
    m2::RectD(3, 23, 257, 42), m2::RectD(600, 23, 800, 42),
    m2::RectD(3, 43, 257, 62), m2::RectD(600, 43, 800, 62),
    m2::RectD(3, 65, 257, 84), m2::RectD(600, 65, 800, 84),
    m2::RectD(3, 87, 257, 106), m2::RectD(600, 87, 800, 106)
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    size_t const count = theTree.GetSize();

    theTree.ReplaceIf(arr[i], arr[i], &compare_false);
    TEST_EQUAL ( theTree.GetSize(), count + 1, () );

    theTree.ReplaceIf(arr1[i], arr1[i], &compare_false);
    TEST_EQUAL ( theTree.GetSize(), count + 1, () );
  }

  vector<m2::RectD> test;
  theTree.ForEach(MakeBackInsertFunctor(test));

  TEST_EQUAL(ARRAY_SIZE(arr), test.size(), ());
  for (size_t i = 0; i < test.size(); ++i)
    TEST_EQUAL(test[i], arr[i], ());
}

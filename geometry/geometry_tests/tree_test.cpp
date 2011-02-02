#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"

#include "../tree4d.hpp"

namespace
{
  typedef m2::RectD R;

  typedef m4::Tree<R> tree_t;

  bool compare_true(R const &, R const &) { return true; }
  bool compare_false(R const &, R const &) { return false; }
}

UNIT_TEST(Tree4D_Smoke)
{
  tree_t theTree;

  R arr[] = {
    R(0, 0, 1, 1),
    R(1, 1, 2, 2),
    R(2, 2, 3, 3)
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    theTree.ReplaceIf(arr[i], arr[i], &compare_true);

  vector<R> test;
  theTree.ForEach(MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  test.clear();
  R const searchR(1.5, 1.5, 1.5, 1.5);
  theTree.ForEachInRect(searchR, MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], arr[1], ());

  R const replaceR(0.5, 0.5, 2.5, 2.5);
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

  R arr[] = {
    R(8, 13, 554, 32), R(555, 13, 700, 32),
    R(8, 33, 554, 52), R(555, 33, 700, 52),
    R(8, 54, 554, 73), R(555, 54, 700, 73),
    R(8, 76, 554, 95), R(555, 76, 700, 95)
  };

  R arr1[] = {
    R(3, 23, 257, 42), R(600, 23, 800, 42),
    R(3, 43, 257, 62), R(600, 43, 800, 62),
    R(3, 65, 257, 84), R(600, 65, 800, 84),
    R(3, 87, 257, 106), R(600, 87, 800, 106)
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    size_t const count = theTree.GetSize();

    theTree.ReplaceIf(arr[i], arr[i], &compare_false);
    TEST_EQUAL ( theTree.GetSize(), count + 1, () );

    theTree.ReplaceIf(arr1[i], arr1[i], &compare_false);
    TEST_EQUAL ( theTree.GetSize(), count + 1, () );
  }

  vector<R> test;
  theTree.ForEach(MakeBackInsertFunctor(test));

  TEST_EQUAL(ARRAY_SIZE(arr), test.size(), ());
  for (size_t i = 0; i < test.size(); ++i)
    TEST_EQUAL(test[i], arr[i], ());
}

namespace 
{
  void CheckInRect(R const * arr, size_t count, R const & searchR, size_t expected)
  {
    tree_t theTree;

    for (size_t i = 0; i < count; ++i)
      theTree.Add(arr[i], arr[i]);

    vector<R> test;
    theTree.ForEachInRect(searchR, MakeBackInsertFunctor(test));

    TEST_EQUAL(test.size(), expected, ());
  }
}

UNIT_TEST(Tree4D_ForEachInRect)
{
  {
    R arr[] = {
      R(0, 0, 1, 1), R(5, 5, 10, 10), R(-1, -1, 0, 0), R(-10, -10, -5, -5)
    };
    CheckInRect(arr, ARRAY_SIZE(arr), R(1, 1, 5, 5), 0);
    CheckInRect(arr, ARRAY_SIZE(arr), R(-5, -5, -1, -1), 0);
    CheckInRect(arr, ARRAY_SIZE(arr), R(3, 3, 3, 3), 0);
    CheckInRect(arr, ARRAY_SIZE(arr), R(-3, -3, -3, -3), 0);

    CheckInRect(arr, ARRAY_SIZE(arr), R(0.5, 0.5, 0.5, 0.5), 1);
    CheckInRect(arr, ARRAY_SIZE(arr), R(8, 8, 8, 8), 1);
    CheckInRect(arr, ARRAY_SIZE(arr), R(-0.5, -0.5, -0.5, -0.5), 1);
    CheckInRect(arr, ARRAY_SIZE(arr), R(-8, -8, -8, -8), 1);

    CheckInRect(arr, ARRAY_SIZE(arr), R(0.5, 0.5, 5.5, 5.5), 2);
    CheckInRect(arr, ARRAY_SIZE(arr), R(-5.5, -5.5, -0.5, -0.5), 2);
  }
}

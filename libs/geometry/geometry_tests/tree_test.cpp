#include "testing/testing.hpp"

#include "geometry/tree4d.hpp"

#include <functional>

namespace tree_test
{
using namespace std;
using R = m2::RectD;

struct traits_t
{
  m2::RectD LimitRect(m2::RectD const & r) const { return r; }
};

using Tree = m4::Tree<R, traits_t>;

template <typename T>
bool RTrue(T const &, T const &)
{
  return true;
}

template <typename T>
bool RFalse(T const &, T const &)
{
  return false;
}

UNIT_TEST(Tree4D_Smoke)
{
  Tree theTree;

  R arr[] = {R(0, 0, 1, 1), R(1, 1, 2, 2), R(2, 2, 3, 3)};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    theTree.ReplaceAllInRect(arr[i], &RTrue<R>);

  vector<R> test;
  theTree.ForEach(base::MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  test.clear();
  R const searchR(1.5, 1.5, 1.5, 1.5);
  theTree.ForEachInRect(searchR, base::MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], arr[1], ());

  R const replaceR(0.5, 0.5, 2.5, 2.5);
  theTree.ReplaceAllInRect(replaceR, &RTrue<R>);

  test.clear();
  theTree.ForEach(base::MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
  TEST_EQUAL(test[0], replaceR, ());

  test.clear();
  theTree.ForEachInRect(searchR, base::MakeBackInsertFunctor(test));
  TEST_EQUAL(1, test.size(), ());
}

UNIT_TEST(Tree4D_ForAnyInRect)
{
  Tree theTree;

  R arr[] = {R(0, 0, 1, 1), R(0, 0, 5, 5), R(1, 1, 2, 2), R(1, 1, 6.5, 6.5), R(2, 2, 3, 3), R(2, 2, 7, 7)};
  for (auto const & r : arr)
    theTree.Add(r);

  TEST(theTree.ForAnyInRect(R(0, 0, 100, 100), [](R const & rect) { return rect.maxX() > 4; }), ());
  TEST(theTree.ForAnyInRect(R(0, 0, 100, 100), [](R const & rect) { return rect.maxX() > 5; }), ());
  TEST(theTree.ForAnyInRect(R(0, 0, 100, 100), [](R const & rect) { return rect.maxX() > 0; }), ());
  TEST(!theTree.ForAnyInRect(R(0, 0, 100, 100), [](R const & rect) { return rect.maxX() > 10; }), ());
}

UNIT_TEST(Tree4D_ReplaceAllInRect)
{
  Tree theTree;

  R arr[] = {R(8, 13, 554, 32), R(555, 13, 700, 32), R(8, 33, 554, 52), R(555, 33, 700, 52),
             R(8, 54, 554, 73), R(555, 54, 700, 73), R(8, 76, 554, 95), R(555, 76, 700, 95)};

  R arr1[] = {R(3, 23, 257, 42), R(600, 23, 800, 42), R(3, 43, 257, 62),  R(600, 43, 800, 62),
              R(3, 65, 257, 84), R(600, 65, 800, 84), R(3, 87, 257, 106), R(600, 87, 800, 106)};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    size_t const count = theTree.GetSize();

    theTree.ReplaceAllInRect(arr[i], &RFalse<R>);
    TEST_EQUAL(theTree.GetSize(), count + 1, ());

    theTree.ReplaceAllInRect(arr1[i], &RFalse<R>);
    TEST_EQUAL(theTree.GetSize(), count + 1, ());
  }

  vector<R> test;
  theTree.ForEach(base::MakeBackInsertFunctor(test));

  TEST_EQUAL(ARRAY_SIZE(arr), test.size(), ());
  for (size_t i = 0; i < test.size(); ++i)
    TEST_EQUAL(test[i], arr[i], ());
}

namespace
{
void CheckInRect(R const * arr, size_t count, R const & searchR, size_t expected)
{
  Tree theTree;

  for (size_t i = 0; i < count; ++i)
    theTree.Add(arr[i], arr[i]);

  vector<R> test;
  theTree.ForEachInRect(searchR, base::MakeBackInsertFunctor(test));

  TEST_EQUAL(test.size(), expected, ());
}
}  // namespace

UNIT_TEST(Tree4D_ForEachInRect)
{
  R arr[] = {R(0, 0, 1, 1), R(5, 5, 10, 10), R(-1, -1, 0, 0), R(-10, -10, -5, -5)};
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

struct TestObj : public m2::RectD
{
  int m_id;

  TestObj(double minX, double minY, double maxX, double maxY, int id) : m2::RectD(minX, minY, maxX, maxY), m_id(id) {}

  bool operator==(TestObj const & r) const { return m_id == r.m_id; }
};

UNIT_TEST(Tree4D_ReplaceEqual)
{
  typedef TestObj T;
  m4::Tree<T, traits_t> theTree;

  T arr[] = {T(0, 0, 1, 1, 1), T(1, 1, 2, 2, 2), T(2, 2, 3, 3, 3)};

  // 1.
  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    theTree.ReplaceEqualInRect(arr[i], equal_to<T>(), &RTrue<T>);

  vector<T> test;
  theTree.ForEach(base::MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  // 2.
  theTree.ReplaceEqualInRect(T(0, 0, 3, 3, 2), equal_to<T>(), &RFalse<T>);

  test.clear();
  theTree.ForEach(base::MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  auto i = find(test.begin(), test.end(), T(1, 1, 2, 2, 2));
  TEST_EQUAL(R(*i), R(1, 1, 2, 2), ());

  // 3.
  theTree.ReplaceEqualInRect(T(0, 0, 3, 3, 2), equal_to<T>(), &RTrue<T>);

  test.clear();
  theTree.ForEach(base::MakeBackInsertFunctor(test));
  TEST_EQUAL(3, test.size(), ());

  i = find(test.begin(), test.end(), T(1, 1, 2, 2, 2));
  TEST_EQUAL(R(*i), R(0, 0, 3, 3), ());
}
}  // namespace tree_test

#include "testing/testing.hpp"

#include "base/macros.hpp"

#include "base/stl_add.hpp"

#include <deque>


UNIT_TEST(STLAdd_IsSorted)
{
  TEST(IsSorted(static_cast<int*>(0), static_cast<int*>(0)), ());
  int v1[] = { 1, 3, 5 };
  int const v2[] = { 1, 3, 2 };
  TEST(!IsSorted(&v2[0], &v2[0] + ARRAY_SIZE(v2)), ());
  TEST(IsSorted(&v1[0], &v1[0] + ARRAY_SIZE(v1)), ());
  TEST(IsSorted(&v1[0], &v1[0] + 0), ());
  TEST(IsSorted(&v1[0], &v1[0] + 1), ());
  TEST(IsSorted(&v1[0], &v1[0] + 2), ());
}

namespace 
{
  struct EqualZero
  {
    bool operator() (int x) { return (x == 0); }
  };

  template <class ContT> void CheckNoZero(ContT & c, typename ContT::iterator i)
  {
    c.erase(i, c.end());
    TEST(find_if(c.begin(), c.end(), EqualZero()) == c.end(), ());
  }
}

UNIT_TEST(STLAdd_RemoveIfKeepValid)
{
  {
    std::vector<int> v;
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());
  }

  {
    std::vector<int> v;
    v.push_back(0);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.begin(), ());

    v.push_back(0);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.begin(), ());

    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 1, ());

    v.push_back(1);
    v.push_back(0);
    v.push_back(0);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 2, ());

    v.push_back(0);
    v.push_back(0);
    v.push_back(1);
    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 4, ());
  }

  {
    std::deque<int> v;
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    v.push_back(0);
    v.push_back(1);
    CheckNoZero(v, RemoveIfKeepValid(v.begin(), v.end(), EqualZero()));
    TEST_EQUAL(v.size(), 4, ());
  }
}

namespace
{
  template <class T, size_t N1, size_t N2, size_t N3>
  void CheckAccumulateIntervals(size_t & idTest,
                                std::pair<T, T> (&arr1)[N1],
                                std::pair<T, T> (&arr2)[N2],
                                std::pair<T, T> (&arr3)[N3])
  {
    std::vector<std::pair<T, T> > res;
    AccumulateIntervals1With2(arr1, arr1 + N1, arr2, arr2 + N2, back_inserter(res));

    ++idTest;
    TEST_EQUAL(N3, res.size(), ("Test", idTest, res));
    TEST(equal(res.begin(), res.end(), arr3), ("Test", idTest, res));
  }
}

UNIT_TEST(STLAdd_AccumulateIntervals)
{
  typedef std::pair<int, int> T;
  size_t idTest = 0;

  // bound cases
  {
    std::vector<T> res;
    T arr[] = { T(10, 20) };

    res.clear();
    AccumulateIntervals1With2(arr, arr + 1, arr, arr, back_inserter(res));
    TEST_EQUAL(res.size(), 1, ());

    res.clear();
    AccumulateIntervals1With2(arr, arr, arr, arr + 1, back_inserter(res));
    TEST_EQUAL(res.size(), 0, ());
  }

  // check splice overlapped
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(19, 31) };
    T res[] = { T(10, 40) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }

  // check skip not overlapped
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(0, 9), T(21, 29), T(41, 50) };
    T res[2] = { T(10, 20), T(30, 40) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(1, 2), T(3, 4), T(5, 6),
                 T(21, 22), T(23, 24), T(25, 26),
                 T(41, 42), T(43, 44), T(45, 46)
               };
    T res[] = { T(10, 20), T(30, 40) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }

  // check equal bounds
  {
    T arr1[] = { T(20, 30) };
    T arr2[] = { T(10, 20), T(30, 40) };
    T res[] = { T(20, 30) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(20, 30) };
    T res[] = { T(10, 20), T(30, 40) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }

  // check large overlap interval
  {
    T arr1[] = { T(10, 20), T(30, 40), T(50, 60) };
    T arr2[] = { T(0, 100) };
    T res[] = { T(0, 100) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }
  {
    T arr1[] = { T(0, 100) };
    T arr2[] = { T(10, 20), T(30, 40), T(50, 60) };
    T res[] = { T(0, 100) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }

  // check splice overlapped
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(5, 15), T(35, 45) };
    T res[] = { T(5, 20), T(30, 45) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }
  {
    T arr1[] = { T(10, 20), T(30, 40) };
    T arr2[] = { T(1, 2), T(3, 4), T(5, 15),
                 T(17, 25), T(26, 27), T(28, 32),
                 T(38, 45), T(46, 50)
               };
    T res[] = { T(5, 25), T(28, 45) };
    CheckAccumulateIntervals(idTest, arr1, arr2, res);
  }
}

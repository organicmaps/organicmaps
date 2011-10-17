#include "../../testing/testing.hpp"
#include "../macros.hpp"
#include "../stl_add.hpp"

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
    vector<int> v;
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());

    v.push_back(1);
    TEST(RemoveIfKeepValid(v.begin(), v.end(), EqualZero()) == v.end(), ());
  }

  {
    vector<int> v;
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
}

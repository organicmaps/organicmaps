#include "testing/testing.hpp"

#include "generator/tesselator.hpp"

#include "base/logging.hpp"

using namespace std;

namespace
{
  typedef m2::PointD P;

  class DoDump
  {
    size_t & m_count;
  public:
    DoDump(size_t & count) : m_count(count)
    {
      m_count = 0;
    }
    void operator() (P const & p1, P const & p2, P const & p3)
    {
      ++m_count;
      LOG(LINFO, (p1, p2, p3));
    }
  };

  size_t RunTest(list<vector<P> > const & l)
  {
    tesselator::TrianglesInfo info;
    int const trianglesCount = tesselator::TesselateInterior(l, info);

    size_t count;
    info.ForEachTriangle(DoDump(count));
    TEST_EQUAL(count, static_cast<size_t>(trianglesCount), ());
    return count;
  }

  size_t RunTess(P const * arr, size_t count)
  {
    list<vector<P> > l;
    l.push_back(vector<P>());
    l.back().assign(arr, arr + count);

    return RunTest(l);
  }
}

UNIT_TEST(Tesselator_SelfISect)
{
  P arr[] = { P(0, 0), P(0, 4), P(4, 4), P(1, 1), P(1, 3), P(4, 0), P(0, 0) };
  TEST_EQUAL(6, RunTess(arr, ARRAY_SIZE(arr)), ());
}

UNIT_TEST(Tesselator_Odd)
{
  P arr[] = { P(-100, -100), P(100, 100), P(100, -100) };

  size_t const count = ARRAY_SIZE(arr);

  list<vector<P> > l;
  l.push_back(vector<P>());
  l.back().assign(arr, arr + count);
  l.push_back(vector<P>());
  l.back().assign(arr, arr + count);

  TEST_EQUAL(0, RunTest(l), ());

  P arr1[] = { P(-100, -100), P(100, -100), P(100, 100), P(-100, 100) };

  l.push_back(vector<P>());
  l.back().assign(arr1, arr1 + ARRAY_SIZE(arr1));

  TEST_EQUAL(2, RunTest(l), ());
}

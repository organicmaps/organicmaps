#include "../../testing/testing.hpp"

#include "../tesselator.hpp"

#include "../../base/logging.hpp"


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

  size_t RunTess(P const * arr, size_t count)
  {
    list<vector<P> > l;
    l.push_back(vector<P>());
    l.back().assign(arr, arr + count);

    tesselator::TrianglesInfo info;
    tesselator::TesselateInterior(l, info);

    info.ForEachTriangle(DoDump(count));
    return count;
  }
}

UNIT_TEST(TesselatorSelfISect_Smoke)
{
  P arr[] = { P(0, 0), P(0, 4), P(4, 4), P(1, 1), P(1, 3), P(4, 0), P(0, 0) };
  TEST_EQUAL(6, RunTess(arr, ARRAY_SIZE(arr)), ());
}

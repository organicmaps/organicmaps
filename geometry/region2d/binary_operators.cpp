#include "../../base/SRC_FIRST.hpp"

#include "binary_operators.hpp"
#include "boost_concept.hpp"


namespace m2
{
  void IntersectRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res)
  {
    using namespace boost::polygon;
    using namespace boost::polygon::operators;

    res.clear();
    res += r1 * r2;
  }
}

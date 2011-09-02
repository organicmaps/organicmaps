#include "../../base/SRC_FIRST.hpp"

#include "binary_operators.hpp"
#include "boost_concept.hpp"


namespace m2
{
  using namespace boost::polygon;
  using namespace boost::polygon::operators;

  void IntersectRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res)
  {
    res += r1 * r2;
  }

  void DiffRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res)
  {
    res += boost::polygon::operators::operator-(r1, r2);
  }
}

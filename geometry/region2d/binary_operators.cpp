#include "geometry/region2d/binary_operators.hpp"
#include "geometry/region2d/boost_concept.hpp"

#include <cstddef>

using namespace std;

namespace m2
{
using namespace boost::polygon;
using namespace boost::polygon::operators;

void SpliceRegions(vector<RegionI> & src, vector<RegionI> & res)
{
  for (size_t i = 0; i < src.size(); ++i)
  {
    res.push_back(RegionI());
    res.back().Swap(src[i]);
  }
}

void IntersectRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res)
{
  vector<RegionI> local;
  local += (r1 * r2);
  SpliceRegions(local, res);
}

void DiffRegions(RegionI const & r1, RegionI const & r2, vector<RegionI> & res)
{
  vector<RegionI> local;
  local += boost::polygon::operators::operator-(r1, r2);
  SpliceRegions(local, res);
}
}  // namespace m2

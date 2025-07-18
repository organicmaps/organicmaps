#include "geometry/region2d/binary_operators.hpp"
#include "geometry/region2d/boost_concept.hpp"

namespace m2
{
void SpliceRegions(std::vector<RegionI> & src, std::vector<RegionI> & res)
{
  for (size_t i = 0; i < src.size(); ++i)
  {
    res.push_back(RegionI());
    res.back().Swap(src[i]);
  }
}

void IntersectRegions(RegionI const & r1, RegionI const & r2, MultiRegionI & res)
{
  MultiRegionI local;
  using namespace boost::polygon::operators;
  local += (r1 * r2);
  SpliceRegions(local, res);
}

MultiRegionI IntersectRegions(RegionI const & r1, MultiRegionI const & r2)
{
  MultiRegionI local;
  using namespace boost::polygon::operators;
  local += (r1 * r2);
  return local;
}

void DiffRegions(RegionI const & r1, RegionI const & r2, MultiRegionI & res)
{
  MultiRegionI local;
  using namespace boost::polygon::operators;
  local += boost::polygon::operators::operator-(r1, r2);
  SpliceRegions(local, res);
}

void AddRegion(RegionI const & r, MultiRegionI & res)
{
  using namespace boost::polygon::operators;
  res += r;
}

uint64_t Area(MultiRegionI const & rgn)
{
  uint64_t area = 0;
  for (auto const & r : rgn)
    area += r.CalculateArea();
  return area;
}
}  // namespace m2

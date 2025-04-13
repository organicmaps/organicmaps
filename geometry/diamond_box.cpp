#include "geometry/diamond_box.hpp"

namespace m2
{
DiamondBox::DiamondBox(std::vector<PointD> const & points)
{
  for (auto const & p : points)
    Add(p);
}
}  // namespace m2

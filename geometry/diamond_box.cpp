#include "geometry/diamond_box.hpp"

using namespace std;

namespace m2
{
DiamondBox::DiamondBox(vector<PointD> const & points)
{
  for (auto const & p : points)
    Add(p);
}

string DebugPrint(DiamondBox const & dbox)
{
  return "DiamondBox [ " + ::DebugPrint(dbox.Points()) + " ]";
}
}  // namespace m2

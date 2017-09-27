#include "geometry/bounding_box.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace m2
{
BoundingBox::BoundingBox(vector<PointD> const & points)
{
  for (auto const & p : points)
    Add(p);
}

void BoundingBox::Add(double x, double y)
{
  m_min.x = min(m_min.x, x);
  m_min.y = min(m_min.y, y);
  m_max.x = max(m_max.x, x);
  m_max.y = max(m_max.y, y);
}

bool BoundingBox::HasPoint(double x, double y) const
{
  return x >= m_min.x && x <= m_max.x && y >= m_min.y && y <= m_max.y;
}

string DebugPrint(BoundingBox const & bbox)
{
  ostringstream os;
  os << "BoundingBox [ ";
  os << "min: " << DebugPrint(bbox.Min()) << ", ";
  os << "max: " << DebugPrint(bbox.Max());
  os << " ]";
  return os.str();
}
}  // namespace m2

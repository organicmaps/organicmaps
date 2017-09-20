#include "geometry/bounding_box.hpp"

#include <algorithm>

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
  m_minX = min(m_minX, x);
  m_minY = min(m_minY, y);
  m_maxX = max(m_maxX, x);
  m_maxY = max(m_maxY, y);
}

bool BoundingBox::HasPoint(double x, double y) const
{
  return x >= m_minX && x <= m_maxX && y >= m_minY && y <= m_maxY;
}
}  // namespace m2

#include "geometry/bbox.hpp"

#include <algorithm>
using namespace std;

namespace m2
{
BBox::BBox(std::vector<m2::PointD> const & points)
{
  for (auto const & p : points)
    Add(p);
}

void BBox::Add(double x, double y)
{
  m_minX = std::min(m_minX, x);
  m_minY = std::min(m_minY, y);
  m_maxX = std::max(m_maxX, x);
  m_maxY = std::max(m_maxY, y);
}

bool BBox::IsInside(double x, double y) const
{
  return x >= m_minX && x <= m_maxX && y >= m_minY && y <= m_maxY;
}
}  // namespace m2

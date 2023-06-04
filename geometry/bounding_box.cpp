#include "geometry/bounding_box.hpp"

#include <algorithm>  // std::min, std::max

namespace m2
{
BoundingBox::BoundingBox(std::vector<PointD> const & points)
{
  for (auto const & p : points)
    Add(p);
}

void BoundingBox::Add(double x, double y)
{
  m_min.x = std::min(m_min.x, x);
  m_min.y = std::min(m_min.y, y);
  m_max.x = std::max(m_max.x, x);
  m_max.y = std::max(m_max.y, y);
}

bool BoundingBox::HasPoint(double x, double y) const
{
  return x >= m_min.x && x <= m_max.x && y >= m_min.y && y <= m_max.y;
}

bool BoundingBox::HasPoint(double x, double y, double eps) const
{
  return x >= m_min.x - eps && x <= m_max.x + eps && y >= m_min.y - eps && y <= m_max.y + eps;
}
}  // namespace m2

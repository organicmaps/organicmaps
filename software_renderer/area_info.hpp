#pragma once

#include "geometry/point2d.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace software_renderer
{
class AreaInfo
{
  m2::PointD m_center;

public:
  std::vector<m2::PointD> m_path;

  void reserve(size_t ptsCount)
  {
    m_path.reserve(ptsCount);
  }

  void swap(AreaInfo & r)
  {
    m_path.swap(r.m_path);
    std::swap(m_center, r.m_center);
  }

  void push_back(m2::PointD const & pt)
  {
    m_path.push_back(pt);
  }

  size_t size() const { return m_path.size(); }

  void SetCenter(m2::PointD const & p) { m_center = p; }
  m2::PointD GetCenter() const { return m_center; }
};
}  // namespace software_renderer

inline void swap(software_renderer::AreaInfo & p1, software_renderer::AreaInfo & p2)
{
  p1.swap(p2);
}

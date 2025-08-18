#pragma once

#include "geometry/bounding_box.hpp"
#include "geometry/calipers_box.hpp"
#include "geometry/diamond_box.hpp"
#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <vector>

namespace indexer
{
// Approximates city boundary.
// City points are inside the intersection of boxes.
struct CityBoundary
{
  CityBoundary() = default;

  explicit CityBoundary(std::vector<m2::PointD> const & ps) : m_bbox(ps), m_cbox(ps), m_dbox(ps) {}

  bool HasPoint(m2::PointD const & p) const { return m_bbox.HasPoint(p) && m_dbox.HasPoint(p) && m_cbox.HasPoint(p); }

  bool HasPoint(double x, double y) const { return HasPoint(m2::PointD(x, y)); }

  bool HasPoint(m2::PointD const & p, double eps) const
  {
    return m_bbox.HasPoint(p, eps) && m_dbox.HasPoint(p, eps) && m_cbox.HasPoint(p, eps);
  }

  bool HasPoint(double x, double y, double eps) const { return HasPoint(m2::PointD(x, y), eps); }

  bool operator==(CityBoundary const & rhs) const
  {
    return m_bbox == rhs.m_bbox && m_cbox == rhs.m_cbox && m_dbox == rhs.m_dbox;
  }

  DECLARE_VISITOR(visitor(m_bbox), visitor(m_cbox), visitor(m_dbox))
  DECLARE_DEBUG_PRINT(CityBoundary)

  m2::BoundingBox m_bbox;
  m2::CalipersBox m_cbox;
  m2::DiamondBox m_dbox;
};
}  // namespace indexer

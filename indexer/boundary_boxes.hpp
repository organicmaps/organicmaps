#pragma once

#include "geometry/bounding_box.hpp"
#include "geometry/calipers_box.hpp"
#include "geometry/diamond_box.hpp"
#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace indexer
{
struct BoundaryBoxes
{
  BoundaryBoxes() = default;

  BoundaryBoxes(std::vector<m2::PointD> const & ps) : m_bbox(ps), m_cbox(ps), m_dbox(ps) {}

  bool HasPoint(m2::PointD const & p) const
  {
    return m_bbox.HasPoint(p) && m_dbox.HasPoint(p) && m_cbox.HasPoint(p);
  }

  bool HasPoint(double x, double y) const { return HasPoint(m2::PointD(x, y)); }

  bool operator==(BoundaryBoxes const & rhs) const
  {
    return m_bbox == rhs.m_bbox && m_cbox == rhs.m_cbox && m_dbox == rhs.m_dbox;
  }

  DECLARE_VISITOR(visitor(m_bbox), visitor(m_cbox), visitor(m_dbox))

  m2::BoundingBox m_bbox;
  m2::CalipersBox m_cbox;
  m2::DiamondBox m_dbox;
};

inline std::string DebugPrint(BoundaryBoxes const & boxes)
{
  std::ostringstream os;
  os << "BoundaryBoxes [ ";
  os << DebugPrint(boxes.m_bbox) << ", ";
  os << DebugPrint(boxes.m_cbox) << ", ";
  os << DebugPrint(boxes.m_dbox) << " ]";
  return os.str();
}
}  // namespace indexer

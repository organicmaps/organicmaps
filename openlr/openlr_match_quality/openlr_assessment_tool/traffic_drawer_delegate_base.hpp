#pragma once

#include <geometry/point2d.hpp>

namespace openlr
{
/// This class is used to delegate segments drawing to the DrapeEngine.
class TrafficDrawerDelegateBase
{
public:
  virtual ~TrafficDrawerDelegateBase() = default;

  virtual void SetViewportCenter(m2::PointD const & center) = 0;

  virtual void DrawDecodedSegments(std::vector<m2::PointD> const & points) = 0;
  virtual void DrawEncodedSegment(std::vector<m2::PointD> const & points) = 0;
  virtual void DrawGoldenPath(std::vector<m2::PointD> const & points) = 0;

  virtual void ClearGoldenPath() = 0;
  virtual void ClearAllPaths() = 0;

  virtual void VisualizePoints(std::vector<m2::PointD> const & points) = 0;
  virtual void ClearAllVisualizedPoints() = 0;
};
}  // namespace openlr

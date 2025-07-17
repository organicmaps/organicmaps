#pragma once

#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"

#include <cstddef>
#include <vector>

namespace search
{
struct ProjectionOnStreet
{
  ProjectionOnStreet();

  // Nearest point on the street to a given point.
  m2::PointD m_proj;

  // Distance in meters from a given point to |m_proj|.
  double m_distMeters;

  // Index of the street segment that |m_proj| belongs to.
  size_t m_segIndex;

  // When true, the point is located to the right from the projection
  // segment, otherwise, the point is located to the left from the
  // projection segment.
  bool m_projSign;
};

class ProjectionOnStreetCalculator
{
public:
  explicit ProjectionOnStreetCalculator(std::vector<m2::PointD> const & points);

  // Finds nearest point on the street to the |point|. If such point
  // is located within |m_maxDistMeters|, stores projection in |proj|
  // and returns true. Otherwise, returns false and does not modify
  // |proj|.
  bool GetProjection(m2::PointD const & point, ProjectionOnStreet & proj) const;

private:
  std::vector<m2::ParametrizedSegment<m2::PointD>> m_segments;
};
}  // namespace search

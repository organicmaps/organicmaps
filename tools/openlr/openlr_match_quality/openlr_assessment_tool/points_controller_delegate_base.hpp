#pragma once

#include "indexer/feature.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <vector>

namespace openlr
{
using FeaturePoint = std::pair<FeatureID, size_t>;

/// This class is responsible for collecting junction points and
/// checking user's clicks.
class PointsControllerDelegateBase
{
public:
  enum class ClickType
  {
    Miss,
    Add,
    Remove
  };

  virtual ~PointsControllerDelegateBase() = default;

  virtual std::vector<m2::PointD> GetAllJunctionPointsInViewport() const = 0;
  /// Returns all junction points at a given location in the form of feature id and
  /// point index in the feature.
  virtual std::pair<std::vector<FeaturePoint>, m2::PointD> GetCandidatePoints(m2::PointD const & p) const = 0;
  // Returns all points that are one step reachable from |p|.
  virtual std::vector<m2::PointD> GetReachablePoints(m2::PointD const & p) const = 0;

  virtual ClickType CheckClick(m2::PointD const & clickPoint, m2::PointD const & lastClickedPoint,
                               std::vector<m2::PointD> const & reachablePoints) const = 0;
};
}  // namespace openlr

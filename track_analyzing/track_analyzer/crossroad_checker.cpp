#include "track_analyzing/track_analyzer/crossroad_checker.hpp"

#include "routing/joint.hpp"
#include "routing/segment.hpp"

#include "routing_common/vehicle_model.hpp"

#include "base/assert.hpp"

#include <algorithm>

using namespace std;

namespace
{
using namespace routing;

bool IsHighwayLink(HighwayType type)
{
  switch (type)
  {
  case HighwayType::HighwayMotorwayLink:
  case HighwayType::HighwayTrunkLink:
  case HighwayType::HighwayPrimaryLink:
  case HighwayType::HighwaySecondaryLink:
  case HighwayType::HighwayTertiaryLink:
    return true;
  default:
    return false;
  }

  UNREACHABLE();
}

bool IsBigHighway(HighwayType type)
{
  switch (type)
  {
  case HighwayType::HighwayMotorway:
  case HighwayType::HighwayTrunk:
  case HighwayType::HighwayPrimary:
  case HighwayType::HighwaySecondary:
  case HighwayType::HighwayTertiary:
    return true;
  default:
    return false;
  }

  UNREACHABLE();
}

bool FromSmallerToBigger(HighwayType lhs, HighwayType rhs)
{
  CHECK_NOT_EQUAL(lhs, rhs, ());

  static std::array<HighwayType, 5> constexpr kHighwayTypes = {
      HighwayType::HighwayTertiary,
      HighwayType::HighwaySecondary,
      HighwayType::HighwayPrimary,
      HighwayType::HighwayTrunk,
      HighwayType::HighwayMotorway
  };

  auto const lhsIt = find(kHighwayTypes.begin(), kHighwayTypes.end(), lhs);
  auto const rhsIt = find(kHighwayTypes.begin(), kHighwayTypes.end(), rhs);
  if (lhsIt == kHighwayTypes.end() && rhsIt != kHighwayTypes.end())
    return true;

  if (lhsIt != kHighwayTypes.end() && rhsIt == kHighwayTypes.end())
    return false;

  return lhsIt < rhsIt;
}
}  // namespace

namespace routing
{
IsCrossroadChecker::CrossroadInfo IsCrossroadChecker::operator()(Segment const & current, Segment const & next) const
{
  IsCrossroadChecker::CrossroadInfo ret{};
  if (current == next)
    return ret;

  auto const currentSegmentFeatureId = current.GetFeatureId();
  auto const currentSegmentHwType = m_geometry.GetRoad(currentSegmentFeatureId).GetHighwayType();
  auto const nextSegmentFeatureId = next.GetFeatureId();
  auto const nextSegmentHwType = m_geometry.GetRoad(nextSegmentFeatureId).GetHighwayType();
  auto const currentRoadPoint = current.GetRoadPoint(true /* isFront */);
  auto const jointId = m_indexGraph.GetJointId(currentRoadPoint);
  if (jointId == Joint::kInvalidId)
    return ret;

  if (currentSegmentFeatureId != nextSegmentFeatureId && currentSegmentHwType != nextSegmentHwType)
  {
    // Changing highway type.
    if (IsHighwayLink(currentSegmentHwType))
      ++ret[base::Underlying(Type::FromLink)];

    if (IsHighwayLink(nextSegmentHwType))
      ++ret[base::Underlying(Type::ToLink)];

    // It's move without links.
    if (!ret[base::Underlying(Type::FromLink)] && !ret[base::Underlying(Type::ToLink)])
    {
      bool const currentIsBig = IsBigHighway(currentSegmentHwType);
      bool const nextIsBig = IsBigHighway(nextSegmentHwType);
      if (currentIsBig && !nextIsBig)
      {
        ++ret[base::Underlying(Type::TurnFromBiggerToSmaller)];
      }
      else if (!currentIsBig && nextIsBig)
      {
        ++ret[base::Underlying(Type::TurnFromSmallerToBigger)];
      }
      else if (currentIsBig && nextIsBig)
      {
        // Both roads are big but one is bigger.
        auto const type = FromSmallerToBigger(currentSegmentHwType, nextSegmentHwType) ?
                          Type::TurnFromSmallerToBigger : Type::TurnFromBiggerToSmaller;
        ++ret[base::Underlying(type)];
      }
    }
  }

  auto const nextRoadPoint = next.GetRoadPoint(false /* isFront */);
  m_indexGraph.ForEachPoint(jointId, [&](RoadPoint const & point) {
    // Check for already included roads.
    auto const pointFeatureId = point.GetFeatureId();
    if (pointFeatureId == currentSegmentFeatureId || pointFeatureId == nextSegmentFeatureId)
      return;

    auto const & roadGeometry = m_geometry.GetRoad(pointFeatureId);
    auto const pointHwType = roadGeometry.GetHighwayType();
    if (pointHwType == nextSegmentHwType)
    {
      // Is the same road but parted on different features.
      if (roadGeometry.IsEndPointId(point.GetPointId()) &&
          roadGeometry.IsEndPointId(nextRoadPoint.GetPointId()))
        return;
    }

    if (IsHighwayLink(pointHwType))
    {
      ++ret[base::Underlying(Type::IntersectionWithLink)];
      return;
    }

    auto const type = IsBigHighway(pointHwType) ? Type::IntersectionWithBig : Type::IntersectionWithSmall;
    ++ret[base::Underlying(type)];
  });

  return ret;
}

// static
void IsCrossroadChecker::MergeCrossroads(IsCrossroadChecker::CrossroadInfo const & from,
                                         IsCrossroadChecker::CrossroadInfo & to)
{
  for (size_t i = 0; i < from.size(); ++i)
    to[i] += from[i];
}
}  // namespace routing

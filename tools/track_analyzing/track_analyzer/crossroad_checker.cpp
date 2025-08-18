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
  case HighwayType::HighwayTertiaryLink: return true;
  default: return false;
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
  case HighwayType::HighwayTertiary: return true;
  default: return false;
  }

  UNREACHABLE();
}

bool FromSmallerToBigger(HighwayType lhs, HighwayType rhs)
{
  CHECK_NOT_EQUAL(lhs, rhs, ());

  static std::array<HighwayType, 5> constexpr kHighwayTypes = {
      HighwayType::HighwayTertiary, HighwayType::HighwaySecondary, HighwayType::HighwayPrimary,
      HighwayType::HighwayTrunk, HighwayType::HighwayMotorway};

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
IsCrossroadChecker::Type IsCrossroadChecker::operator()(Segment const & current, Segment const & next) const
{
  auto const currentSegmentFeatureId = current.GetFeatureId();
  auto const currentSegmentHwType = m_geometry.GetRoad(currentSegmentFeatureId).GetHighwayType();
  auto const nextSegmentFeatureId = next.GetFeatureId();
  auto const nextSegmentHwType = m_geometry.GetRoad(nextSegmentFeatureId).GetHighwayType();
  auto const currentRoadPoint = current.GetRoadPoint(true /* isFront */);
  auto const jointId = m_indexGraph.GetJointId(currentRoadPoint);
  if (jointId == Joint::kInvalidId)
    return Type::Count;

  bool const isCurrentLink = IsHighwayLink(*currentSegmentHwType);
  bool const isNextLink = IsHighwayLink(*nextSegmentHwType);
  bool const isCurrentBig = IsBigHighway(*currentSegmentHwType);
  bool const isNextBig = IsBigHighway(*nextSegmentHwType);

  if (currentSegmentFeatureId != nextSegmentFeatureId && currentSegmentHwType != nextSegmentHwType)
  {
    // Changing highway type.
    if (isCurrentLink && !isNextLink && isNextBig)
      return Type::TurnFromSmallerToBigger;

    if (!isCurrentLink && isNextLink && isCurrentBig)
      return Type::TurnFromBiggerToSmaller;

    // It's move without links.
    if (!isCurrentLink && !isNextLink)
    {
      if (isCurrentBig && !isNextBig)
        return Type::TurnFromBiggerToSmaller;
      if (!isCurrentBig && isNextBig)
        return Type::TurnFromSmallerToBigger;
    }
  }

  Type retType = Type::Count;
  auto const nextRoadPoint = next.GetRoadPoint(false /* isFront */);
  m_indexGraph.ForEachPoint(jointId, [&](RoadPoint const & point)
  {
    if (retType != IsCrossroadChecker::Type::Count)
      return;

    // Check for already included roads.
    auto const pointFeatureId = point.GetFeatureId();
    if (pointFeatureId == currentSegmentFeatureId || pointFeatureId == nextSegmentFeatureId)
      return;

    auto const & roadGeometry = m_geometry.GetRoad(pointFeatureId);
    auto const pointHwType = roadGeometry.GetHighwayType();
    if (currentSegmentHwType == pointHwType)
      return;

    if (pointHwType == nextSegmentHwType)
    {
      // Is the same road but parted on different features.
      if (roadGeometry.IsEndPointId(point.GetPointId()) && roadGeometry.IsEndPointId(nextRoadPoint.GetPointId()))
        return;
    }

    if (isCurrentLink && IsBigHighway(*pointHwType))
    {
      retType = Type::IntersectionWithBig;
      return;
    }

    if (FromSmallerToBigger(*currentSegmentHwType, *pointHwType))
    {
      retType = Type::IntersectionWithBig;
      return;
    }
  });

  return retType;
}

// static
void IsCrossroadChecker::MergeCrossroads(Type from, CrossroadInfo & to)
{
  if (from != Type::Count)
    ++to[base::Underlying(from)];
}

// static
void IsCrossroadChecker::MergeCrossroads(IsCrossroadChecker::CrossroadInfo const & from,
                                         IsCrossroadChecker::CrossroadInfo & to)
{
  for (size_t i = 0; i < from.size(); ++i)
    to[i] += from[i];
}

std::string DebugPrint(IsCrossroadChecker::Type type)
{
  switch (type)
  {
  case IsCrossroadChecker::Type::TurnFromSmallerToBigger: return "TurnFromSmallerToBigger";
  case IsCrossroadChecker::Type::TurnFromBiggerToSmaller: return "TurnFromBiggerToSmaller";
  case IsCrossroadChecker::Type::IntersectionWithBig: return "IntersectionWithBig";
  case IsCrossroadChecker::Type::Count: return "Count";
  }
  UNREACHABLE();
}
}  // namespace routing

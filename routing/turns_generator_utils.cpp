#include "routing/turns_generator_utils.hpp"

namespace routing
{
namespace turns
{

using namespace ftypes;

bool IsHighway(HighwayClass hwClass, bool isLink)
{
  return (hwClass == HighwayClass::Trunk || hwClass == HighwayClass::Primary) &&
         !isLink;
}

bool IsSmallRoad(HighwayClass hwClass)
{
  return hwClass == HighwayClass::LivingStreet ||
         hwClass == HighwayClass::Service || hwClass == HighwayClass::Pedestrian;
}

int CalcDiffRoadClasses(HighwayClass const left, HighwayClass const right)
{
  return static_cast<int>(left) - static_cast<int>(right);
}

template <class T>
T FindDirectionByAngle(vector<pair<double, T>> const & lowerBounds, double const angle)
{
  ASSERT_GREATER_OR_EQUAL(angle, -180., (angle));
  ASSERT_LESS_OR_EQUAL(angle, 180., (angle));
  ASSERT(!lowerBounds.empty(), ());
  ASSERT(is_sorted(lowerBounds.rbegin(), lowerBounds.rend(), base::LessBy(&pair<double, T>::first)),
         ());

  for (auto const & lower : lowerBounds)
  {
    if (angle >= lower.first)
      return lower.second;
  }

  ASSERT(false, ("The angle is not covered by the table. angle = ", angle));
  return T::None;
}

CarDirection InvertDirection(CarDirection const dir)
{
  switch (dir)
  {
    case CarDirection::TurnSlightRight:
      return CarDirection::TurnSlightLeft;
    case CarDirection::TurnRight:
      return CarDirection::TurnLeft;
    case CarDirection::TurnSharpRight:
      return CarDirection::TurnSharpLeft;
    case CarDirection::TurnSlightLeft:
      return CarDirection::TurnSlightRight;
    case CarDirection::TurnLeft:
      return CarDirection::TurnRight;
    case CarDirection::TurnSharpLeft:
      return CarDirection::TurnSharpRight;
    default:
      return dir;
  };
}

CarDirection RightmostDirection(double const angle)
{
  static vector<pair<double, CarDirection>> const kLowerBounds = {
      {145., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {10., CarDirection::TurnSlightRight},
      // For sure it's incorrect to give directions TurnLeft or TurnSlighLeft if we need the rightmost turn.
      // So GoStraight direction is given even for needed sharp left turn when other turns on the left and are more sharp.
      // The reason: the rightmost turn is the most straight one here.
      {-180., CarDirection::GoStraight}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

CarDirection LeftmostDirection(double const angle)
{
  return InvertDirection(RightmostDirection(-angle));
}

CarDirection IntermediateDirection(double const angle)
{
  static vector<pair<double, CarDirection>> const kLowerBounds = {
      {145., CarDirection::TurnSharpRight},
      {50., CarDirection::TurnRight},
      {10., CarDirection::TurnSlightRight},
      {-10., CarDirection::GoStraight},
      {-50., CarDirection::TurnSlightLeft},
      {-145., CarDirection::TurnLeft},
      {-180., CarDirection::TurnSharpLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

PedestrianDirection IntermediateDirectionPedestrian(double const angle)
{
  static vector<pair<double, PedestrianDirection>> const kLowerBounds = {
      {10.0, PedestrianDirection::TurnRight},
      {-10.0, PedestrianDirection::GoStraight},
      {-180.0, PedestrianDirection::TurnLeft}};

  return FindDirectionByAngle(kLowerBounds, angle);
}

// TurnInfo ----------------------------------------------------------------------------------------
bool TurnInfo::IsSegmentsValid() const
{
  if (m_ingoing->m_path.empty() || m_outgoing->m_path.empty())
  {
    LOG(LWARNING, ("Some turns can't load the geometry."));
    return false;
  }
  return true;
}

}  // namespace turns
}  // namespace routing
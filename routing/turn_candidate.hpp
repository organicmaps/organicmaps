#pragma once

#include "routing/turns.hpp"

#include "base/math.hpp"

#include "std/vector.hpp"

namespace ftypes
{
enum class HighwayClass;
}

namespace routing
{
namespace turns
{
/*!
 * \brief The TurnCandidate struct contains information about possible ways from a junction.
 */
struct TurnCandidate
{
  /*!
   * angle is an angle of the turn in degrees. It means angle is 180 minus
   * an angle between the current edge and the edge of the candidate. A counterclockwise rotation.
   * The current edge is an edge which belongs the route and located before the junction.
   * angle belongs to the range [-180; 180];
   */
  double angle;
  /*!
   * |m_segmentRange| is a possible way from the junction.
   * |m_segmentRange| contains mwm id, feature id, segment id and direction in case of A*.
   */
  SegmentRange m_segmentRange;
  /*!
   * \brief highwayClass field for the road class caching. Because feature reading is a long
   * function.
   */
  ftypes::HighwayClass highwayClass;

  TurnCandidate(double a, SegmentRange const & segmentRange, ftypes::HighwayClass c)
    : angle(a), m_segmentRange(segmentRange), highwayClass(c)
  {
  }

  bool IsAlmostEqual(TurnCandidate const & rhs) const
  {
    double constexpr kEpsilon = 0.01;
    return my::AlmostEqualAbs(angle, rhs.angle, kEpsilon) && m_segmentRange == rhs.m_segmentRange &&
           highwayClass == rhs.highwayClass;
  }
};

inline bool IsAlmostEqual(vector<TurnCandidate> const & lhs, vector<TurnCandidate> const & rhs)
{
  if (lhs.size() != rhs.size())
    return false;

  for (size_t i = 0; i < lhs.size(); ++i)
  {
    if (!lhs[i].IsAlmostEqual(rhs[i]))
      return false;
  }
  return true;
}

struct TurnCandidates
{
  vector<TurnCandidate> candidates;
  bool isCandidatesAngleValid;

  explicit TurnCandidates(bool angleValid = true) : isCandidatesAngleValid(angleValid) {}

  bool IsAlmostEqual(TurnCandidates const & rhs) const
  {
    return turns::IsAlmostEqual(candidates, rhs.candidates) &&
           isCandidatesAngleValid == rhs.isCandidatesAngleValid;
  }
};
}  // namespace routing
}  // namespace turns

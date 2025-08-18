#pragma once

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"
#include "generator/mini_roundabout_info.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
struct RoundaboutUnit
{
  uint64_t m_roadId = 0;
  m2::PointD m_location;
  FeatureParams::Types m_roadTypes;
};

class MiniRoundaboutTransformer
{
public:
  MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                            feature::AffiliationInterface const & affiliation);
  MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                            feature::AffiliationInterface const & affiliation, double radiusMeters);

  void AddRoad(feature::FeatureBuilder && road);

  /// \brief Generates ways with junction=roundabout.
  /// These features are obtained from points with highway=mini_roundabout.
  void ProcessRoundabouts(std::function<void(feature::FeatureBuilder const &)> const & fn);

  void SetLeftHandTraffic(bool isLeftHand) { m_leftHandTraffic = isLeftHand; }

private:
  using PointsT = feature::FeatureBuilder::PointSeq;
  feature::FeatureBuilder CreateRoundaboutFb(PointsT && way, uint32_t roadType);

  /// \brief Splits |road| in two parts: part before the |roundabout| and after.
  /// Returns second road to |newRoads| - the artificial one.
  PointsT CreateSurrogateRoad(RoundaboutUnit const & roundaboutOnRoad, PointsT & roundaboutCircle, PointsT & road,
                              PointsT::iterator & itPointUpd) const;

  /// \brief Creates new point and adds it to |roundabout| circle and to the |road|.
  bool AddRoundaboutToRoad(RoundaboutUnit const & roundaboutOnRoad, PointsT & roundaboutCircle, PointsT & road,
                           std::vector<feature::FeatureBuilder> & newRoads) const;

  std::vector<MiniRoundaboutInfo> const & m_roundabouts;
  double const m_radiusMercator = 0.0;
  feature::AffiliationInterface const * m_affiliation = nullptr;
  std::unordered_map<base::GeoObjectId, feature::FeatureBuilder> m_roads;
  // Skip 2 bytes to satisfy base::GeoObjectId constraints.
  uint64_t m_newWayId = 1ULL << (63 - 16);
  bool m_leftHandTraffic = false;
};

/// \brief Calculates Euclidean distance between 2 points on plane.
double DistanceOnPlain(m2::PointD const & a, m2::PointD const & b);

/// \returns The point that is located on the segment (|source|, |target|) and lies in |dist|
/// or less from |target|.
m2::PointD GetPointAtDistFromTarget(m2::PointD const & source, m2::PointD const & target, double dist);

/// \brief Creates a regular polygon with |verticesCount| inscribed in a circle with |center| and
/// |radiusMercator|. The polygon is rotated by an angle |initAngleDeg| CCW.
/// \returns vector of polygon vertices.
std::vector<m2::PointD> PointToPolygon(m2::PointD const & center, double radiusMercator, size_t verticesCount = 12,
                                       double initAngleDeg = 0.0);

/// \brief Inserts point (which lies on circle) between 2 points already present in the circle.
void AddPointToCircle(std::vector<m2::PointD> & circle, m2::PointD const & point);
}  // namespace generator

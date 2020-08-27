#pragma once

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"
#include "generator/mini_roundabout_info.hpp"
#include "generator/osm_element.hpp"

#include "coding/point_coding.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <string>
#include <vector>

namespace generator
{
struct RoundaboutUnit
{
  uint64_t m_roadId = 0;
  m2::PointD m_location;
  FeatureParams::Types m_roadTypes;
};

class MiniRoundaboutData
{
public:
  MiniRoundaboutData(std::vector<MiniRoundaboutInfo> && data);

  bool RoadExists(feature::FeatureBuilder const & fb) const;
  std::vector<MiniRoundaboutInfo> const & GetData() const;

private:
  std::vector<MiniRoundaboutInfo> m_data;
  std::vector<uint64_t> m_ways;
};

/// \brief Loads info about mini_roundabouts from binary source.
MiniRoundaboutData ReadDataMiniRoundabout(std::string const & intermediateFilePath);

class MiniRoundaboutTransformer
{
public:
  explicit MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                                     feature::AffiliationInterface const & affiliation);
  explicit MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                                     feature::AffiliationInterface const & affiliation,
                                     double radiusMeters);

  void AddRoad(feature::FeatureBuilder && road);

  /// \brief Generates ways with junction=roundabout.
  /// These features are obtained from points with highway=mini_roundabout.
  std::vector<feature::FeatureBuilder> ProcessRoundabouts();

  void SetLeftHandTraffic(bool isLeftHand) { m_leftHandTraffic = isLeftHand; }

private:
  /// \brief Sets |road_type| to |found_type| if it is a more important road type.
  void UpdateRoadType(FeatureParams::Types const & foundTypes, uint32_t & roadType);

  /// \brief Splits |road| in two parts: part before the |roundabout| and after.
  /// Returns second road to |newRoads| - the artificial one.
  feature::FeatureBuilder::PointSeq CreateSurrogateRoad(
      RoundaboutUnit const & roundaboutOnRoad, std::vector<m2::PointD> & roundaboutCircle,
      feature::FeatureBuilder::PointSeq & road,
      feature::FeatureBuilder::PointSeq::iterator & itPointUpd);

  /// \brief Creates new point and adds it to |roundabout| circle and to the |road|.
  bool AddRoundaboutToRoad(RoundaboutUnit const & roundaboutOnRoad,
                           std::vector<m2::PointD> & roundaboutCircle,
                           feature::FeatureBuilder::PointSeq & road,
                           std::vector<feature::FeatureBuilder> & newRoads);


  std::vector<MiniRoundaboutInfo> const & m_roundabouts;
  double const m_radiusMercator = 0.0;
  feature::AffiliationInterface const * m_affiliation = nullptr;
  std::vector<feature::FeatureBuilder> m_roads;
  bool m_leftHandTraffic = false;
};

/// \brief Calculates Euclidean distance between 2 points on plane.
double DistanceOnPlain(m2::PointD const & a, m2::PointD const & b);

/// \returns The point that is located on the segment (|source|, |target|) and lies in |dist|
/// or less from |target|.
m2::PointD GetPointAtDistFromTarget(m2::PointD const & source, m2::PointD const & target,
                                    double dist);

/// \brief Creates a regular polygon with |verticesCount| inscribed in a circle with |center| and
/// |radiusMercator|. The polygon is rotated by an angle |initAngleDeg| CCW.
/// \returns vector of polygon vertices.
std::vector<m2::PointD> PointToPolygon(m2::PointD const & center, double radiusMercator,
                                       size_t verticesCount = 12, double initAngleDeg = 0.0);

/// \brief Inserts point (which lies on circle) between 2 points already present in the circle.
void AddPointToCircle(std::vector<m2::PointD> & circle, m2::PointD const & point);
}  // namespace generator

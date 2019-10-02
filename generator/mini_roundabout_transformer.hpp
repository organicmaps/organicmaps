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
class MiniRoundaboutTransformer
{
public:
  explicit MiniRoundaboutTransformer(std::string const & intermediateFilePath);
  MiniRoundaboutTransformer(std::string const & intermediateFilePath, double radiusMeters);

  /// \brief Adds ways with junction=roundabout to |fbs|.
  /// These features are obtained from points with highway=mini_roundabout.
  void ProcessRoundabouts(feature::CountriesFilesIndexAffiliation const & affiliation,
                          std::vector<feature::FeatureBuilder> & fbs);

private:
  /// \brief Loads info about mini_roundabouts from binary source.
  void ReadData(std::string const & intermediateFilePath);

  /// \brief Sets |road_type| to |found_type| if it is a more important road type.
  void UpdateRoadType(FeatureParams::Types const & foundTypes, uint32_t & roadType);

  /// \brief Creates new point and add it to |roundabout| circle and to the |road|.
  bool AddRoundaboutToRoad(m2::PointD const & center, std::vector<m2::PointD> & roundabout,
                           feature::FeatureBuilder::PointSeq & road);

  std::vector<MiniRoundaboutInfo> m_roundabouts;
  double const m_radiusMercator = 0.0;
};

/// \brief Calculates Euclidean distance between 2 points on plane.
double DistanceOnPlain(m2::PointD const & a, m2::PointD const & b);

/// \returns The point that is located on the segment (|segPoint|, |target|) and lies in |r| or less
/// from |target|.
m2::PointD TrimSegment(m2::PointD const & segPoint, m2::PointD const & target, double r);

/// \brief Creates a regular polygon with |verticesCount| inscribed in a circle with |center| and
/// |radiusMercator|. The polygon is rotated by an angle |initAngleDeg| CCW.
/// \returns vector of polygon vertices.
std::vector<m2::PointD> PointToPolygon(m2::PointD const & center, double radiusMercator,
                                       size_t verticesCount = 12, double initAngleDeg = 0.0);

/// \brief Inserts point (which lies on circle) between 2 points already present in the circle.
void AddPointToCircle(std::vector<m2::PointD> & circle, m2::PointD const & point);
}  // namespace generator

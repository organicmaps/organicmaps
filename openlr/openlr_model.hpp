#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace openlr
{
// The form of way (FOW) describes the physical road type of a line.
enum class FormOfWay
{
  // The physical road type is unknown.
  Undefined,
  // A road permitted for motorized vehicles only in combination with a prescribed minimum speed.
  // It has two or more physically separated carriageways and no single level-crossings.
  Motorway,
  // A road with physically separated carriageways regardless of the number of lanes.
  // If a road is also a motorway, it should be coded as such and not as a multiple carriageway.
  MultipleCarriageway,
  // All roads without separate carriageways are considered as roads with a single carriageway.
  SingleCarriageway,
  // A road which forms a ring on which traffic traveling in only one direction is allowed.
  Roundabout,
  // An open area (partly) enclosed by roads which is used for non-traffic purposes
  // and which is not a Roundabout.
  Trafficsquare,
  // A road especially designed to enter or leave a line.
  Sliproad,
  // The physical road type is known, but does not fit into one of the other categories.
  Other,
  // A path only allowed for bikes.
  BikePath,
  // A path only allowed for pedestrians.
  Footpath,
  NotAValue
};

// The functional road class (FRC) of a line is a road classification based on the importance
// of the road represented by the line.
enum class FunctionalRoadClass
{
  // Main road, highest importance.
  FRC0,
  // First class road.
  FRC1,

  // Other road classes.

  FRC2,
  FRC3,
  FRC4,
  FRC5,
  FRC6,
  FRC7,
  NotAValue
};

// LinearSegment structure may be filled from olr:locationReference xml tag,
// from coordinates tag or not valid.
enum class LinearSegmentSource
{
  NotValid,
  FromLocationReferenceTag,
  FromCoordinatesTag,
};

struct LocationReferencePoint
{
  // Coordinates of the point of interest.
  ms::LatLon m_latLon = ms::LatLon::Zero();
  // The bearing (BEAR) describes the angle between the true North and a line which is defined
  // by the coordinate of the LocationReferencePoint and a coordinate which is BEARDIST along
  // the line defined by the LocationReference-point attributes.
  // For more information see OpenLR-Whitepaper `Bearing' section.
  uint8_t m_bearing = 0;
  FunctionalRoadClass m_functionalRoadClass = FunctionalRoadClass::NotAValue;
  FormOfWay m_formOfWay = FormOfWay::NotAValue;

  // The distance to next point field describes the distance to the next LocationReferencePoint
  // in the topological connection of the LocationReferencePoints. The distance is measured in meters
  // and is calculated along the location reference path between two subsequent LR-points.
  // The last LRP has the distance value 0.
  // Should not be used in the last point of a segment.
  uint32_t m_distanceToNextPoint = 0;
  // The lowest FunctionalRoadClass to the next point (LFRCNP) is the lowest FunctionalRoadClass
  // value which appears in the location reference path between two consecutive LR-points.
  // This information could be used to limit the number of road classes which need to be
  // scanned during the decoding.
  // Should not be used in the last point of a segment.
  FunctionalRoadClass m_lfrcnp = FunctionalRoadClass::NotAValue;
  bool m_againstDrivingDirection = false;
};

struct LinearLocationReference
{
  std::vector<LocationReferencePoint> m_points;
  uint32_t m_positiveOffsetMeters = 0;
  uint32_t m_negativeOffsetMeters = 0;
};

struct LinearSegment
{
  static auto constexpr kInvalidSegmentId = std::numeric_limits<uint32_t>::max();

  std::vector<m2::PointD> GetMercatorPoints() const;
  std::vector<LocationReferencePoint> const & GetLRPs() const;
  std::vector<LocationReferencePoint> & GetLRPs();

  LinearSegmentSource m_source = LinearSegmentSource::NotValid;
  // TODO(mgsergio): Think of using openlr::PartnerSegmentId
  uint32_t m_segmentId = kInvalidSegmentId;
  // TODO(mgsergio): Make sure that one segment cannot contain
  // more than one location reference.
  LinearLocationReference m_locationReference;
  uint32_t m_segmentLengthMeters = 0;
  // uint32_t m_segmentRefSpeed;  Always null in partners data. (No docs found).
};

std::string DebugPrint(LinearSegmentSource source);
}  // namespace openlr

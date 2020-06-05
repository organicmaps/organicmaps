#pragma once

#include "transit/transit_entities.hpp"

#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include "base/newtype.hpp"
#include "base/visitor.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

// This is the implementation of the new transit classes which support not only subway, but also
// public transport types from GTFS. Since it is experimental it exists at one time with
// subway classes for handling networks, stops and other transit entities. When the time comes this
// transit implementation will completely replace the subway classes, they will be removed, and the
// experimental namespace will be also removed.
namespace transit
{
namespace experimental
{
#define DECLARE_TRANSIT_TYPES_FRIENDS \
  template <class Sink>               \
  friend class Serializer;            \
  template <class Source>             \
  friend class Deserializer;          \
  template <typename Sink>            \
  friend class FixedSizeSerializer;   \
  template <typename Sink>            \
  friend class FixedSizeDeserializer;

using FeatureId = uint32_t;
using OsmId = uint64_t;

FeatureId constexpr kInvalidFeatureId = std::numeric_limits<FeatureId>::max();
OsmId constexpr kInvalidOsmId = std::numeric_limits<OsmId>::max();

class SingleMwmSegment
{
public:
  SingleMwmSegment() = default;
  SingleMwmSegment(FeatureId featureId, uint32_t segmentIdx, bool forward);

  FeatureId GetFeatureId() const { return m_featureId; }
  uint32_t GetSegmentIdx() const { return m_segmentIdx; }
  bool IsForward() const { return m_forward; }

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(SingleMwmSegment, visitor(m_featureId, "feature_id"),
                                  visitor(m_segmentIdx, "segment_idx"),
                                  visitor(m_forward, "forward"))

  FeatureId m_featureId = kInvalidFeatureId;
  uint32_t m_segmentIdx = 0;
  bool m_forward = false;
};

/// \brief This class represents osm id and feature id of the same feature.
class IdBundle
{
public:
  explicit IdBundle(bool serializeFeatureIdOnly);
  IdBundle(FeatureId featureId, OsmId osmId, bool serializeFeatureIdOnly);

  bool operator<(IdBundle const & rhs) const;
  bool operator==(IdBundle const & rhs) const;
  bool operator!=(IdBundle const & rhs) const;
  bool IsValid() const;
  void SetFeatureId(FeatureId featureId);
  void SetOsmId(OsmId osmId);

  FeatureId GetFeatureId() const;
  OsmId GetOsmId() const;
  bool SerializeFeatureIdOnly() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(IdBundle, visitor(m_featureId, "feature_id"),
                                  visitor(m_osmId, "osm_id"))

  FeatureId m_featureId = kInvalidFeatureId;
  OsmId m_osmId = kInvalidOsmId;
  bool m_serializeFeatureIdOnly = true;
};

class TransitHeader
{
  // TODO(o.khlopkova) Add body.
};

class Network
{
public:
  Network() = default;
  Network(TransitId id, Translations const & title);
  explicit Network(TransitId id);

  bool operator<(Network const & rhs) const;
  bool operator==(Network const & rhs) const;
  bool IsValid() const;

  TransitId GetId() const;
  std::string const GetTitle() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Network, visitor(m_id, "id"), visitor(m_title, "title"))

  TransitId m_id = kInvalidTransitId;
  Translations m_title;
};

class Route
{
public:
  Route() = default;
  Route(TransitId id, TransitId networkId, std::string const & routeType,
        Translations const & title, std::string const & color);

  bool operator<(Route const & rhs) const;
  bool operator==(Route const & rhs) const;
  bool IsValid() const;

  TransitId GetId() const;
  std::string const GetTitle() const;
  std::string const & GetType() const;
  std::string const & GetColor() const;
  TransitId GetNetworkId() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Route, visitor(m_id, "id"), visitor(m_networkId, "network_id"),
                                  visitor(m_routeType, "type"), visitor(m_title, "title"),
                                  visitor(m_color, "color"))
  TransitId m_id = kInvalidTransitId;
  TransitId m_networkId = kInvalidTransitId;
  std::string m_routeType;
  Translations m_title;
  std::string m_color;
};

class Line
{
public:
  Line() = default;
  Line(TransitId id, TransitId routeId, ShapeLink shapeLink, Translations const & title,
       Translations const & number, IdList stopIds, std::vector<LineInterval> const & intervals,
       osmoh::OpeningHours const & serviceDays);

  bool operator<(Line const & rhs) const;
  bool operator==(Line const & rhs) const;
  bool IsValid() const;

  TransitId GetId() const;
  std::string GetNumber() const;
  std::string GetTitle() const;
  TransitId GetRouteId() const;
  ShapeLink const & GetShapeLink() const;
  IdList const & GetStopIds() const;
  std::vector<LineInterval> GetIntervals() const;
  osmoh::OpeningHours GetServiceDays() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Line, visitor(m_id, "id"), visitor(m_routeId, "route_id"),
                                  visitor(m_shapeLink, "shape_link"), visitor(m_title, "title"),
                                  visitor(m_number, "number"), visitor(m_stopIds, "stop_ids"),
                                  visitor(m_intervals, "intervals"),
                                  visitor(m_serviceDays.GetRule(), "service_days"))
  TransitId m_id = kInvalidTransitId;
  TransitId m_routeId = kInvalidTransitId;
  ShapeLink m_shapeLink;
  Translations m_title;
  Translations m_number;
  IdList m_stopIds;
  std::vector<LineInterval> m_intervals;
  osmoh::OpeningHours m_serviceDays;
};

class Stop
{
public:
  Stop();
  Stop(TransitId id, FeatureId featureId, OsmId osmId, Translations const & title,
       TimeTable const & timetable, m2::PointD const & point);
  explicit Stop(TransitId id);

  bool operator<(Stop const & rhs) const;
  bool operator==(Stop const & rhs) const;
  bool IsValid() const;

  FeatureId GetId() const;
  FeatureId GetFeatureId() const;
  OsmId GetOsmId() const;
  std::string GetTitle() const;
  TimeTable const & GetTimeTable() const;
  m2::PointD const & GetPoint() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  // TODO(o.khlopkova) add visitor for |m_timetable|.
  DECLARE_VISITOR_AND_DEBUG_PRINT(Stop, visitor(m_id, "id"), visitor(m_ids, "osm_id"),
                                  visitor(m_title, "title"), visitor(m_point, "point"))

  TransitId m_id = kInvalidTransitId;
  IdBundle m_ids;
  Translations m_title;
  TimeTable m_timetable;
  m2::PointD m_point;
};

class Gate
{
public:
  Gate();
  Gate(TransitId id, FeatureId featureId, OsmId osmId, bool entrance, bool exit,
       std::vector<TimeFromGateToStop> const & weights, m2::PointD const & point);

  bool operator<(Gate const & rhs) const;
  bool operator==(Gate const & rhs) const;
  bool IsValid() const;
  void SetBestPedestrianSegment(SingleMwmSegment const & s);

  FeatureId GetFeatureId() const;
  OsmId GetOsmId() const;
  SingleMwmSegment const & GetBestPedestrianSegment() const;
  bool IsEntrance() const;
  bool IsExit() const;
  std::vector<TimeFromGateToStop> const & GetStopsWithWeight() const;
  m2::PointD const & GetPoint() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Gate, visitor(m_id, "id"), visitor(m_ids, "ids"),
                                  visitor(m_bestPedestrianSegment, "best_pedestrian_segment"),
                                  visitor(m_entrance, "entrance"), visitor(m_exit, "exit"),
                                  visitor(m_weights, "weights"), visitor(m_point, "point"))

  TransitId m_id = kInvalidTransitId;
  // |m_ids| contains feature id of a feature which represents gates. Usually it's a
  // point feature.
  IdBundle m_ids;
  // |m_bestPedestrianSegment| is a segment which can be used for pedestrian routing to leave and
  // enter the gate. The segment may be invalid because of map date. If so there's no pedestrian
  // segment which can be used to reach the gate.
  SingleMwmSegment m_bestPedestrianSegment;
  bool m_entrance = true;
  bool m_exit = true;
  std::vector<TimeFromGateToStop> m_weights;
  m2::PointD m_point;
};

class Edge
{
public:
  Edge() = default;
  Edge(TransitId stop1Id, TransitId stop2Id, EdgeWeight weight, TransitId lineId, bool transfer,
       ShapeLink const & shapeLink);

  bool operator<(Edge const & rhs) const;
  bool operator==(Edge const & rhs) const;
  bool operator!=(Edge const & rhs) const;
  bool IsValid() const;
  void SetWeight(EdgeWeight weight);

  TransitId GetStop1Id() const;
  TransitId GetStop2Id() const;
  EdgeWeight GetWeight() const;
  TransitId GetLineId() const;
  bool IsTransfer() const;
  ShapeLink const & GetShapeLink() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Edge, visitor(m_stop1Id, "stop1_id"),
                                  visitor(m_stop2Id, "stop2_id"), visitor(m_weight, "weight"),
                                  visitor(m_isTransfer, "is_transfer"),
                                  visitor(m_lineId, "line_id"), visitor(m_shapeLink, "shape_link"))

  TransitId m_stop1Id = kInvalidTransitId;
  TransitId m_stop2Id = kInvalidTransitId;
  EdgeWeight m_weight = 0;
  bool m_isTransfer = false;
  TransitId m_lineId = kInvalidTransitId;
  ShapeLink m_shapeLink;
};

class Transfer
{
public:
  Transfer() = default;
  Transfer(TransitId id, m2::PointD const & point, IdList const & stopIds);

  bool operator<(Transfer const & rhs) const;
  bool operator==(Transfer const & rhs) const;
  bool IsValid() const;

  TransitId GetId() const;
  m2::PointD const & GetPoint() const;
  IdList const & GetStopIds() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Transfer, visitor(m_id, "id"), visitor(m_point, "point"),
                                  visitor(m_stopIds, "stop_ids"))

  TransitId m_id = kInvalidTransitId;
  m2::PointD m_point;
  IdList m_stopIds;
};

class Shape
{
public:
  Shape() = default;
  Shape(TransitId id, std::vector<m2::PointD> const & polyline);

  bool operator<(Shape const & rhs) const;
  bool operator==(Shape const & rhs) const;
  bool IsValid() const;

  TransitId GetId() const;
  std::vector<m2::PointD> const & GetPolyline() const;

private:
  DECLARE_TRANSIT_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Shape, visitor(m_id, "id"), visitor(m_polyline, "polyline"))

  TransitId m_id;
  std::vector<m2::PointD> m_polyline;
};
}  // namespace experimental
}  // namespace transit

#pragma once

#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace routing
{
namespace transit
{
using Anchor = uint8_t;
using FeatureId = uint32_t;
using LineId = uint32_t;
using NetworkId = uint32_t;
using OsmId = uint64_t;
using StopId = uint64_t;
using TransferId = uint64_t;
using Weight = double;
using Ranges = std::vector<std::vector<StopId>>;

Anchor constexpr kInvalidAnchor = std::numeric_limits<Anchor>::max();
std::string const kInvalidColor = std::string("");
FeatureId constexpr kInvalidFeatureId = std::numeric_limits<FeatureId>::max();
LineId constexpr kInvalidLineId = std::numeric_limits<LineId>::max();
NetworkId constexpr kInvalidNetworkId = std::numeric_limits<NetworkId>::max();
OsmId constexpr kInvalidOsmId = std::numeric_limits<OsmId>::max();
StopId constexpr kInvalidStopId = std::numeric_limits<StopId>::max();
TransferId constexpr kInvalidTransferId = std::numeric_limits<TransferId>::max();
// Note. Weight may be a default param at json. The default value should be saved as uint32_t in mwm anyway.
// To convert double to uint32_t at better accuracy |kInvalidWeight| should be close to real weight.
Weight constexpr kInvalidWeight = -1.0;

#define DECLARE_TRANSIT_TYPE_FRIENDS                                                  \
  template <class Sink> friend class Serializer;                                      \
  template <class Source> friend class Deserializer;                                  \
  friend class DeserializerFromJson;                                                  \
  template <class Ser, class Deser, class Obj>                                        \
  friend void TestCommonSerialization(Obj const & obj);                               \
  template <typename Sink> friend class FixedSizeSerializer;                          \
  template <typename Sink> friend class FixedSizeDeserializer;                        \

struct TransitHeader
{
  TransitHeader() { Reset(); }
  TransitHeader(uint16_t version, uint32_t stopsOffset, uint32_t gatesOffset, uint32_t edgesOffset,
                uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                uint32_t networksOffset, uint32_t endOffset);

  void Reset();
  bool IsEqualForTesting(TransitHeader const & header) const;
  bool IsValid() const;

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(
      TransitHeader, visitor(m_version, "version"), visitor(m_reserve, "reserve"),
      visitor(m_stopsOffset, "stops"), visitor(m_gatesOffset, "gatesOffset"),
      visitor(m_edgesOffset, "edgesOffset"), visitor(m_transfersOffset, "transfersOffset"),
      visitor(m_linesOffset, "linesOffset"), visitor(m_shapesOffset, "shapesOffset"),
      visitor(m_networksOffset, "networksOffset"), visitor(m_endOffset, "endOffset"))

public:
  uint16_t m_version;
  uint16_t m_reserve;
  uint32_t m_stopsOffset;
  uint32_t m_gatesOffset;
  uint32_t m_edgesOffset;
  uint32_t m_transfersOffset;
  uint32_t m_linesOffset;
  uint32_t m_shapesOffset;
  uint32_t m_networksOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(TransitHeader) == 36, "Wrong header size of transit section.");

/// \brief This class represents osm id and feature id of the same feature.
class FeatureIdentifiers
{
public:
  explicit FeatureIdentifiers(bool serializeFeatureIdOnly);
  FeatureIdentifiers(OsmId osmId, FeatureId const & featureId, bool serializeFeatureIdOnly);

  bool operator<(FeatureIdentifiers const & rhs) const;
  bool operator==(FeatureIdentifiers const & rhs) const;
  bool operator!=(FeatureIdentifiers const & rhs) const { return !(*this == rhs); }
  bool IsValid() const;
  void SetOsmId(OsmId osmId) { m_osmId = osmId; }
  void SetFeatureId(FeatureId featureId) { m_featureId = featureId; }

  OsmId GetOsmId() const { return m_osmId; }
  FeatureId GetFeatureId() const { return m_featureId; }
  bool IsSerializeFeatureIdOnly() const { return m_serializeFeatureIdOnly; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(FeatureIdentifiers, visitor(m_osmId, "osm_id"),
                                  visitor(m_featureId, "feature_id"))

  OsmId m_osmId = kInvalidOsmId;
  FeatureId m_featureId = kInvalidFeatureId;
  bool m_serializeFeatureIdOnly = true;
};

class TitleAnchor
{
public:
  TitleAnchor() = default;
  TitleAnchor(uint8_t minZoom, Anchor anchor);

  bool operator==(TitleAnchor const & titleAnchor) const;
  bool IsEqualForTesting(TitleAnchor const & titleAnchor) const;
  bool IsValid() const;

  uint8_t GetMinZoom() const { return m_minZoom; }
  Anchor const & GetAnchor() const { return m_anchor; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(TitleAnchor, visitor(m_minZoom, "min_zoom"),
                                  visitor(m_anchor, "anchor"))

  uint8_t m_minZoom = scales::UPPER_STYLE_SCALE;
  Anchor m_anchor = kInvalidAnchor;
};

class Stop
{
public:
  Stop() : m_featureIdentifiers(true /* serializeFeatureIdOnly */) {};
  Stop(StopId id, OsmId osmId, FeatureId featureId, TransferId transferId,
       std::vector<LineId> const & lineIds, m2::PointD const & point,
       std::vector<TitleAnchor> const & titleAnchors);

  bool operator<(Stop const & rhs) const { return m_id < rhs.m_id; }
  bool operator==(Stop const & rhs) const { return m_id == rhs.m_id; }
  bool IsEqualForTesting(Stop const & stop) const;
  bool IsValid() const;

  StopId GetId() const { return m_id; }
  FeatureId GetFeatureId() const { return m_featureIdentifiers.GetFeatureId(); }
  TransferId GetTransferId() const { return m_transferId; }
  std::vector<LineId> const & GetLineIds() const { return m_lineIds; }
  m2::PointD const & GetPoint() const { return m_point; }
  std::vector<TitleAnchor> const & GetTitleAnchors() const { return m_titleAnchors; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Stop, visitor(m_id, "id"), visitor(m_featureIdentifiers, "osm_id"),
                                  visitor(m_transferId, "transfer_id"),
                                  visitor(m_lineIds, "line_ids"), visitor(m_point, "point"),
                                  visitor(m_titleAnchors, "title_anchors"))

  StopId m_id = kInvalidStopId;
  FeatureIdentifiers m_featureIdentifiers;
  TransferId m_transferId = kInvalidTransferId;
  std::vector<LineId> m_lineIds;
  m2::PointD m_point;
  std::vector<TitleAnchor> m_titleAnchors;
};

class SingleMwmSegment
{
public:
  SingleMwmSegment() = default;
  SingleMwmSegment(FeatureId featureId, uint32_t segmentIdx, bool forward);
  bool IsEqualForTesting(SingleMwmSegment const & s) const;
  bool IsValid() const;

  FeatureId GetFeatureId() const { return m_featureId; }
  uint32_t GetSegmentIdx() const { return m_segmentIdx; }
  bool GetForward() const { return m_forward; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(SingleMwmSegment, visitor(m_featureId, "feature_id"),
                                  visitor(m_segmentIdx, "segment_idx"),
                                  visitor(m_forward, "forward"))

  FeatureId m_featureId = kInvalidFeatureId;
  uint32_t m_segmentIdx = 0;
  bool m_forward = false;
};

class Gate
{
public:
  Gate() : m_featureIdentifiers(false /* serializeFeatureIdOnly */) {};
  Gate(OsmId osmId, FeatureId featureId, bool entrance, bool exit, double weight,
       std::vector<StopId> const & stopIds, m2::PointD const & point);

  bool operator<(Gate const & rhs) const;
  bool operator==(Gate const & rhs) const;
  bool IsEqualForTesting(Gate const & gate) const;
  bool IsValid() const;
  void SetBestPedestrianSegment(SingleMwmSegment const & s) { m_bestPedestrianSegment = s; };

  FeatureId GetFeatureId() const { return m_featureIdentifiers.GetFeatureId(); }
  OsmId GetOsmId() const { return m_featureIdentifiers.GetOsmId(); }
  SingleMwmSegment const & GetBestPedestrianSegment() const { return m_bestPedestrianSegment; }
  bool GetEntrance() const { return m_entrance; }
  bool GetExit() const { return m_exit; }
  double GetWeight() const { return m_weight; }
  std::vector<StopId> const & GetStopIds() const { return m_stopIds; }
  m2::PointD const & GetPoint() const { return m_point; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Gate, visitor(m_featureIdentifiers, "osm_id"),
                                  visitor(m_bestPedestrianSegment, "best_pedestrian_segment"),
                                  visitor(m_entrance, "entrance"), visitor(m_exit, "exit"),
                                  visitor(m_weight, "weight"), visitor(m_stopIds, "stop_ids"),
                                  visitor(m_point, "point"))

  // |m_featureIdentifiers| contains feature id of a feature which represents gates. Usually it's a point feature.
  FeatureIdentifiers m_featureIdentifiers;
  // |m_bestPedestrianSegment| is a segment which can be used for pedestrian routing to leave and enter the gate.
  // The segment may be invalid because of map date. If so there's no pedestrian segment which can be used
  // to reach the gate.
  SingleMwmSegment m_bestPedestrianSegment;
  bool m_entrance = true;
  bool m_exit = true;
  double m_weight = kInvalidWeight;
  std::vector<StopId> m_stopIds;
  m2::PointD m_point;
};

class ShapeId
{
public:
  ShapeId() = default;
  ShapeId(StopId stop1Id, StopId stop2Id) : m_stop1Id(stop1Id), m_stop2Id(stop2Id) {}

  bool operator<(ShapeId const & rhs) const;
  bool operator==(ShapeId const & rhs) const;
  bool IsEqualForTesting(ShapeId const & rhs) const { return *this == rhs; }

  bool IsValid() const;
  StopId GetStop1Id() const { return m_stop1Id; }
  StopId GetStop2Id() const { return m_stop2Id; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(ShapeId, visitor(m_stop1Id, "stop1_id"),
                                  visitor(m_stop2Id, "stop2_id"))

  StopId m_stop1Id = kInvalidStopId;
  StopId m_stop2Id = kInvalidStopId;
};

class Edge
{
public:
  Edge() = default;
  Edge(StopId stop1Id, StopId stop2Id, double weight, LineId lineId, bool transfer,
       std::vector<ShapeId> const & shapeIds);

  bool operator<(Edge const & rhs) const;
  bool operator==(Edge const & rhs) const;
  bool IsEqualForTesting(Edge const & edge) const;
  bool IsValid() const;
  void SetWeight(double weight) { m_weight = weight; }

  StopId GetStop1Id() const { return m_stop1Id; }
  StopId GetStop2Id() const { return m_stop2Id; }
  double GetWeight() const { return m_weight; }
  LineId GetLineId() const { return m_lineId; }
  bool GetTransfer() const { return m_transfer; }
  std::vector<ShapeId> const & GetShapeIds() const { return m_shapeIds; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Edge, visitor(m_stop1Id, "stop1_id"),
                                  visitor(m_stop2Id, "stop2_id"),
                                  visitor(m_weight, "weight"), visitor(m_lineId, "line_id"),
                                  visitor(m_transfer, "transfer"), visitor(m_shapeIds, "shape_ids"))

  StopId m_stop1Id = kInvalidStopId;
  StopId m_stop2Id = kInvalidStopId;
  double m_weight = kInvalidWeight; // in seconds
  LineId m_lineId = kInvalidLineId;
  bool m_transfer = false;
  std::vector<ShapeId> m_shapeIds;
};

class Transfer
{
public:
  Transfer() = default;
  Transfer(StopId id, m2::PointD const & point, std::vector<StopId> const & stopIds,
           std::vector<TitleAnchor> const & titleAnchors);

  bool operator<(Transfer const & rhs) const { return m_id < rhs.m_id; }
  bool operator==(Transfer const & rhs) const { return m_id == rhs.m_id; }
  bool IsEqualForTesting(Transfer const & transfer) const;
  bool IsValid() const;

  StopId GetId() const { return m_id; }
  m2::PointD const & GetPoint() const { return m_point; }
  std::vector<StopId> const & GetStopIds() const { return m_stopIds; }
  std::vector<TitleAnchor> const & GetTitleAnchors() const { return m_titleAnchors; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Transfer, visitor(m_id, "id"), visitor(m_point, "point"),
                                  visitor(m_stopIds, "stop_ids"),
                                  visitor(m_titleAnchors, "title_anchors"))

  StopId m_id = kInvalidStopId;
  m2::PointD m_point;
  std::vector<StopId> m_stopIds;
  std::vector<TitleAnchor> m_titleAnchors;
};

class StopIdRanges
{
public:
  StopIdRanges() = default;
  explicit StopIdRanges(Ranges const & ids) : m_ids(ids) {}

  bool operator==(StopIdRanges const & rhs) const { return m_ids == rhs.m_ids; }
  bool IsValid() const { return !m_ids.empty(); }

  Ranges const & GetIds() const { return m_ids; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(StopIdRanges, visitor(m_ids, "ids"))

  Ranges m_ids;
};

class Line
{
public:
  Line() = default;
  Line(LineId id, std::string const & number, std::string const & title, std::string const & type,
       std::string const & color, NetworkId networkId, Ranges const & stopIds);

  bool operator<(Line const & rhs) const { return m_id < rhs.m_id; }
  bool operator==(Line const & rhs) const { return m_id == rhs.m_id; }
  bool IsEqualForTesting(Line const & line) const;
  bool IsValid() const;

  LineId GetId() const { return m_id; }
  std::string const & GetNumber() const { return m_number; }
  std::string const & GetTitle() const { return m_title; }
  std::string const & GetType() const { return m_type; }
  std::string const & GetColor() const { return m_color; }
  NetworkId GetNetworkId() const { return m_networkId; }
  Ranges const & GetStopIds() const { return m_stopIds.GetIds(); }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Line, visitor(m_id, "id"), visitor(m_number, "number"),
                                  visitor(m_title, "title"), visitor(m_type, "type"),
                                  visitor(m_color, "color"), visitor(m_networkId, "network_id"),
                                  visitor(m_stopIds, "stop_ids"))

  LineId m_id = kInvalidLineId;
  std::string m_number;
  std::string m_title;
  std::string m_type;
  std::string m_color = kInvalidColor;
  NetworkId m_networkId = kInvalidNetworkId;
  StopIdRanges m_stopIds;
};

class Shape
{
public:
  Shape() = default;
  Shape(ShapeId const & id, std::vector<m2::PointD> const & polyline) : m_id(id), m_polyline(polyline) {}

  bool operator<(Shape const & rhs) const { return m_id < rhs.m_id; }
  bool operator==(Shape const & rhs) const { return m_id == rhs.m_id; }
  bool IsEqualForTesting(Shape const & shape) const;
  bool IsValid() const { return m_id.IsValid() && m_polyline.size() > 1; }

  ShapeId GetId() const { return m_id; }
  std::vector<m2::PointD> const & GetPolyline() const { return m_polyline; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Shape, visitor(m_id, "id"), visitor(m_polyline, "polyline"))

  ShapeId m_id;
  std::vector<m2::PointD> m_polyline;
};

class Network
{
public:
  Network() = default;
  Network(NetworkId id, std::string const & title);

  bool operator<(Network const & rhs) const { return m_id < rhs.m_id; }
  bool operator==(Network const & rhs) const { return m_id == rhs.m_id; }
  bool IsEqualForTesting(Network const & shape) const;
  bool IsValid() const;

  NetworkId GetId() const { return m_id; }
  std::string const & GetTitle() const { return m_title; }

private:
  DECLARE_TRANSIT_TYPE_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Network, visitor(m_id, "id"), visitor(m_title, "title"))

  NetworkId m_id = kInvalidNetworkId;
  std::string m_title;
};

#undef DECLARE_TRANSIT_TYPE_FRIENDS
}  // namespace transit
}  // namespace routing

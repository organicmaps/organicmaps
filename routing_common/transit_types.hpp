#pragma once

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace routing
{
namespace transit
{
using LineId = uint32_t;
using StopId = uint64_t;
using TransferId = uint64_t;
using NetworkId = uint32_t;
using FeatureId = uint32_t;
using ShapeId = uint32_t;
using Weight = double;

LineId constexpr kInvalidLineId = std::numeric_limits<LineId>::max();
StopId constexpr kInvalidStopId = std::numeric_limits<StopId>::max();
TransferId constexpr kInvalidTransferId = std::numeric_limits<TransferId>::max();
NetworkId constexpr kInvalidNetworkId = std::numeric_limits<NetworkId>::max();
FeatureId constexpr kInvalidFeatureId = std::numeric_limits<FeatureId>::max();
ShapeId constexpr kInvalidShapeId = std::numeric_limits<ShapeId>::max();
// Note. Weight may be a default param at json. The default value should be saved as uint32_t in mwm anyway.
// To convert double to uint32_t at better accuracy |kInvalidWeight| should be close to real weight.
Weight constexpr kInvalidWeight = -1.0;

struct TransitHeader
{
  TransitHeader() { Reset(); }
  TransitHeader(uint16_t version, uint32_t gatesOffset, uint32_t edgesOffset,
                uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                uint32_t networksOffset, uint32_t endOffset);
  void Reset();
  bool IsEqualForTesting(TransitHeader const & header) const;

  DECLARE_VISITOR_AND_DEBUG_PRINT(
      TransitHeader, visitor(m_version, "version"), visitor(m_reserve, "reserve"),
      visitor(m_gatesOffset, "gatesOffset"), visitor(m_edgesOffset, "edgesOffset"),
      visitor(m_transfersOffset, "transfersOffset"), visitor(m_linesOffset, "linesOffset"),
      visitor(m_shapesOffset, "shapesOffset"), visitor(m_networksOffset, "networksOffset"),
      visitor(m_endOffset, "endOffset"))

  uint16_t m_version;
  uint16_t m_reserve;
  uint32_t m_gatesOffset;
  uint32_t m_edgesOffset;
  uint32_t m_transfersOffset;
  uint32_t m_linesOffset;
  uint32_t m_shapesOffset;
  uint32_t m_networksOffset;
  uint32_t m_endOffset;
};

static_assert(sizeof(TransitHeader) == 32, "Wrong header size of transit section.");

class Stop
{
public:
  Stop() = default;
  Stop(StopId id, FeatureId featureId, TransferId transferId, std::vector<LineId> const & lineIds,
       m2::PointD const & point);
  bool IsEqualForTesting(Stop const & stop) const;

  StopId GetId() const { return m_id; }
  FeatureId GetFeatureId() const { return m_featureId; }
  TransferId GetTransferId() const { return m_transferId; }
  std::vector<LineId> const & GetLineIds() const { return m_lineIds; }
  m2::PointD const & GetPoint() const { return m_point; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Stop, visitor(m_id, "id"), visitor(m_featureId, "osm_id"),
                                  visitor(m_transferId, "transfer_id"),
                                  visitor(m_lineIds, "line_ids"), visitor(m_point, "point"))

private:
  StopId m_id = kInvalidStopId;
  FeatureId m_featureId = kInvalidFeatureId;
  TransferId m_transferId = kInvalidTransferId;
  std::vector<LineId> m_lineIds;
  m2::PointD m_point;
  // @TODO(bykoianko) It's necessary to add field m_titleAnchors here and implement serialization
  // and deserialization.
};

class Gate
{
public:
  Gate() = default;
  Gate(FeatureId featureId, bool entrance, bool exit, double weight, std::vector<StopId> const & stopIds,
       m2::PointD const & point);
  bool IsEqualForTesting(Gate const & gate) const;

  FeatureId GetFeatureId() const { return m_featureId; }
  std::vector<FeatureId> const & GetPedestrianFeatureIds() const { return m_pedestrianFeatureIds; }
  bool GetEntrance() const { return m_entrance; }
  bool GetExit() const { return m_exit; }
  double GetWeight() const { return m_weight; }
  std::vector<StopId> const & GetStopIds() const { return m_stopIds; }
  m2::PointD const & GetPoint() const { return m_point; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Gate, visitor(m_featureId, "osm_id"),
                                  visitor(m_entrance, "entrance"),
                                  visitor(m_exit, "exit"), visitor(m_weight, "weight"),
                                  visitor(m_stopIds, "stop_ids"), visitor(m_point, "point"))

private:
  // |m_featureId| is feature id of a point feature which represents gates.
  FeatureId m_featureId = kInvalidFeatureId;
  // |m_pedestrianFeatureIds| is linear feature ids which can be used for pedestrian routing
  // to leave (to enter) the gate.
  // @TODO(bykoianko) |m_pedestrianFeatureIds| should be filled after "gates" are deserialized from json
  // to vector of Gate but before serialization to mwm.
  std::vector<FeatureId> m_pedestrianFeatureIds;
  bool m_entrance = true;
  bool m_exit = true;
  double m_weight = kInvalidWeight;
  std::vector<StopId> m_stopIds;
  m2::PointD m_point;
};

class Edge
{
public:
  Edge() = default;
  Edge(StopId startStopId, StopId finishStopId, double weight, LineId lineId, bool transfer,
       std::vector<ShapeId> const & shapeIds);
  bool IsEqualForTesting(Edge const & edge) const;

  StopId GetStartStopId() const { return m_startStopId; }
  StopId GetFinishStopId() const { return m_finishStopId; }
  double GetWeight() const { return m_weight; }
  LineId GetLineId() const { return m_lineId; }
  bool GetTransfer() const { return m_transfer; }
  std::vector<ShapeId> const & GetShapeIds() const { return m_shapeIds; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Edge, visitor(m_startStopId, "start_stop_id"),
                                  visitor(m_finishStopId, "finish_stop_id"),
                                  visitor(m_weight, "weight"), visitor(m_lineId, "line_id"),
                                  visitor(m_transfer, "transfer"), visitor(m_shapeIds, "shape_ids"))

private:
  StopId m_startStopId = kInvalidStopId;
  StopId m_finishStopId = kInvalidStopId;
  double m_weight = kInvalidWeight; // in seconds
  LineId m_lineId = kInvalidLineId;
  bool m_transfer = false;
  std::vector<ShapeId> m_shapeIds;
};

class Transfer
{
public:
  Transfer() = default;
  Transfer(StopId id, m2::PointD const & point, std::vector<StopId> const & stopIds);
  bool IsEqualForTesting(Transfer const & transfer) const;

  StopId GetId() const { return m_id; }
  m2::PointD const & GetPoint() const { return m_point; }
  std::vector<StopId> const & GetStopIds() const { return m_stopIds; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Transfer, visitor(m_id, "id"), visitor(m_point, "point"),
                                  visitor(m_stopIds, "stop_ids"))

private:
  StopId m_id = kInvalidStopId;
  m2::PointD m_point;
  std::vector<StopId> m_stopIds;

  // @TODO(bykoianko) It's necessary to add field m_titleAnchors here and implement serialization
  // and deserialization.
};

class Line
{
public:
  Line() = default;
  Line(LineId id, std::string const & number, std::string const & title, std::string const & type,
       NetworkId networkId, std::vector<StopId> const & stopIds);
  bool IsEqualForTesting(Line const & line) const;

  LineId GetId() const { return m_id; }
  std::string const & GetNumber() const { return m_number; }
  std::string const & GetTitle() const { return m_title; }
  std::string const & GetType() const { return m_type; }
  NetworkId GetNetworkId() const { return m_networkId; }
  std::vector<StopId> const & GetStopIds() const { return m_stopIds; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Line, visitor(m_id, "id"), visitor(m_number, "number"),
                                  visitor(m_title, "title"), visitor(m_type, "type"),
                                  visitor(m_networkId, "network_id"),
                                  visitor(m_stopIds, "stop_ids"))

private:
  LineId m_id = kInvalidLineId;
  std::string m_number;
  std::string m_title;
  std::string m_type;
  NetworkId m_networkId = kInvalidNetworkId;
  std::vector<StopId> m_stopIds;
};

class Shape
{
public:
  Shape() = default;
  Shape(ShapeId id, StopId stop1_id, StopId stop2_id, std::vector<m2::PointD> const & polyline);
  bool IsEqualForTesting(Shape const & shape) const;

  ShapeId GetId() const { return m_id; }
  StopId GetStop1Id() const { return m_stop1_id; }
  StopId GetStop2Id() const { return m_stop2_id; }
  std::vector<m2::PointD> const & GetPolyline() const { return m_polyline; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Shape, visitor(m_id, "id"), visitor(m_stop1_id, "stop1_id"),
                                  visitor(m_stop2_id, "stop2_id"), visitor(m_polyline, "polyline"))

private:
  ShapeId m_id = kInvalidShapeId;
  StopId m_stop1_id = kInvalidStopId;
  StopId m_stop2_id = kInvalidStopId;
  std::vector<m2::PointD> m_polyline;
};

class Network
{
public:
  Network() = default;
  Network(NetworkId id, std::string const & title);
  bool IsEqualForTesting(Network const & shape) const;

  NetworkId GetId() const { return m_id; }
  std::string const & GetTitle() const { return m_title; }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Network, visitor(m_id, "id"), visitor(m_title, "title"))

private:
  NetworkId m_id;
  std::string m_title;
};
}  // namespace transit
}  // namespace routing

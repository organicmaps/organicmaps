#include "routing_common/transit_types.hpp"

#include "routing_common/transit_serdes.hpp"

namespace
{
double constexpr kWeightEqualEpsilon = 1e-2;
double constexpr kPointsEqualEpsilon = 1e-6;
}  // namespace

namespace routing
{
namespace transit
{
// TransitHeader ----------------------------------------------------------------------------------
TransitHeader::TransitHeader(uint16_t version, uint32_t gatesOffset, uint32_t edgesOffset,
                             uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                             uint32_t networksOffset, uint32_t endOffset)
  : m_version(version)
  , m_reserve(0)
  , m_gatesOffset(gatesOffset)
  , m_edgesOffset(edgesOffset)
  , m_transfersOffset(transfersOffset)
  , m_linesOffset(linesOffset)
  , m_shapesOffset(shapesOffset)
  , m_networksOffset(networksOffset)
  , m_endOffset(endOffset)
{
}

void TransitHeader::Reset()
{
  m_version = 0;
  m_reserve = 0;
  m_gatesOffset = 0;
  m_edgesOffset = 0;
  m_transfersOffset = 0;
  m_linesOffset = 0;
  m_shapesOffset = 0;
  m_networksOffset = 0;
  m_endOffset = 0;
}

bool TransitHeader::IsEqualForTesting(TransitHeader const & header) const
{
  return m_version == header.m_version
         && m_reserve == header.m_reserve
         && m_gatesOffset == header.m_gatesOffset
         && m_edgesOffset == header.m_edgesOffset
         && m_transfersOffset == header.m_transfersOffset
         && m_linesOffset == header.m_linesOffset
         && m_shapesOffset == header.m_shapesOffset
         && m_networksOffset == header.m_networksOffset
         && m_endOffset == header.m_endOffset;
}

// Stop -------------------------------------------------------------------------------------------
Stop::Stop(StopId id, FeatureId featureId, TransferId transferId, std::vector<LineId> const & lineIds,
           m2::PointD const & point)
  : m_id(id), m_featureId(featureId), m_transferId(transferId), m_lineIds(lineIds), m_point(point)
{
}

bool Stop::IsEqualForTesting(Stop const & stop) const
{
  double constexpr kPointsEqualEpsilon = 1e-6;
  return m_id == stop.m_id && m_featureId == stop.m_featureId && m_transferId == stop.m_transferId &&
         m_lineIds == stop.m_lineIds && my::AlmostEqualAbs(m_point, stop.m_point, kPointsEqualEpsilon);
}

// Gate -------------------------------------------------------------------------------------------
Gate::Gate(FeatureId featureId, bool entrance, bool exit, double weight,
           std::vector<StopId> const & stopIds, m2::PointD const & point)
  : m_featureId(featureId)
  , m_entrance(entrance)
  , m_exit(exit)
  , m_weight(weight)
  , m_stopIds(stopIds)
  , m_point(point)
{
}

bool Gate::IsEqualForTesting(Gate const & gate) const
{
  return m_featureId == gate.m_featureId && m_entrance == gate.m_entrance &&
         m_exit == gate.m_exit &&
         my::AlmostEqualAbs(m_weight, gate.m_weight, kWeightEqualEpsilon) &&
         m_stopIds == gate.m_stopIds &&
         my::AlmostEqualAbs(m_point, gate.m_point, kPointsEqualEpsilon);
}

// Edge -------------------------------------------------------------------------------------------
Edge::Edge(StopId startStopId, StopId finishStopId, double weight, LineId lineId, bool transfer,
           std::vector<ShapeId> const & shapeIds)
  : m_startStopId(startStopId)
  , m_finishStopId(finishStopId)
  , m_weight(weight)
  , m_lineId(lineId)
  , m_transfer(transfer)
  , m_shapeIds(shapeIds)
{
}

bool Edge::operator<(Edge const & rhs) const
{
  if (m_startStopId != rhs.m_startStopId)
    return m_startStopId < rhs.m_startStopId;
  if (m_finishStopId != rhs.m_finishStopId)
    return m_finishStopId < rhs.m_finishStopId;
  if (m_lineId != rhs.m_lineId)
    return m_lineId < rhs.m_lineId;
  if (m_transfer != rhs.m_transfer)
    return m_transfer < rhs.m_transfer;
  if (m_shapeIds != rhs.m_shapeIds)
    return m_shapeIds < rhs.m_shapeIds;
  if (!my::AlmostEqualAbs(m_weight, rhs.m_weight, kWeightEqualEpsilon))
    return m_weight < rhs.m_weight;
  return false;
}

bool Edge::IsEqualForTesting(Edge const & edge) const { return !(*this < edge || edge < *this); }

// Transfer ---------------------------------------------------------------------------------------
Transfer::Transfer(StopId id, m2::PointD const & point, std::vector<StopId> const & stopIds)
  : m_id(id), m_point(point), m_stopIds(stopIds)
{
}

bool Transfer::IsEqualForTesting(Transfer const & transfer) const
{
  return m_id == transfer.m_id &&
         my::AlmostEqualAbs(m_point, transfer.m_point, kPointsEqualEpsilon) &&
         m_stopIds == transfer.m_stopIds;
}

// Line -------------------------------------------------------------------------------------------
Line::Line(LineId id, std::string const & number, std::string const & title,
           std::string const & type, NetworkId networkId, std::vector<StopId> const & stopIds)
  : m_id(id)
  , m_number(number)
  , m_title(title)
  , m_type(type)
  , m_networkId(networkId)
  , m_stopIds(stopIds)
{
}

bool Line::IsEqualForTesting(Line const & line) const
{
  return m_id == line.m_id && m_number == line.m_number && m_title == line.m_title &&
         m_type == line.m_type && m_networkId == line.m_networkId && m_stopIds == line.m_stopIds;
}

// Shape ------------------------------------------------------------------------------------------
Shape::Shape(ShapeId id, StopId stop1_id, StopId stop2_id, std::vector<m2::PointD> const & polyline)
  : m_id(id), m_stop1_id(stop1_id), m_stop2_id(stop2_id), m_polyline(polyline)
{
}

bool Shape::IsEqualForTesting(Shape const & shape) const
{
  if (!(m_id == shape.m_id && m_stop1_id == shape.m_stop1_id && m_stop2_id == shape.m_stop2_id &&
        m_polyline.size() == shape.m_polyline.size()))
  {
    return false;
  }

  for (size_t i = 0; i < m_polyline.size(); ++i)
  {
    if (!my::AlmostEqualAbs(m_polyline[i], shape.m_polyline[i], kPointsEqualEpsilon))
      return false;
  }
  return true;
}

// Network ----------------------------------------------------------------------------------------
Network::Network(NetworkId id, std::string const & title)
: m_id(id), m_title(title)
{
}

bool Network::IsEqualForTesting(Network const & shape) const
{
  return m_id == shape.m_id && m_title == shape.m_title;
}
}  // namespace transit
}  // namespace routing

#include "transit/experimental/transit_types_experimental.hpp"

#include <tuple>

namespace transit
{
namespace experimental
{
std::string GetTranslation(Translations const & titles)
{
  CHECK(!titles.empty(), ());

  // If there is only one language we return title in this only translation.
  if (titles.size() == 1)
    return titles.begin()->second;

  // Otherwise we try to extract default language for this region.
  auto it = titles.find("default");
  if (it != titles.end())
    return it->second;

  // If there is no default language we return one of the represented translations.
  return titles.begin()->second;
}

// TransitHeader ----------------------------------------------------------------------------------
bool TransitHeader::IsValid() const
{
  return m_stopsOffset <= m_gatesOffset && m_gatesOffset <= m_transfersOffset && m_transfersOffset <= m_linesOffset &&
         m_linesOffset <= m_linesMetadataOffset && m_linesMetadataOffset <= m_shapesOffset &&
         m_shapesOffset <= m_routesOffset && m_routesOffset <= m_networksOffset && m_networksOffset <= m_endOffset;
}

// SingleMwmSegment --------------------------------------------------------------------------------
SingleMwmSegment::SingleMwmSegment(FeatureId featureId, uint32_t segmentIdx, bool forward)
  : m_featureId(featureId)
  , m_segmentIdx(segmentIdx)
  , m_forward(forward)
{}

bool SingleMwmSegment::operator==(SingleMwmSegment const & rhs) const
{
  return std::tie(m_featureId, m_segmentIdx, m_forward) == std::tie(rhs.m_featureId, rhs.m_segmentIdx, rhs.m_forward);
}

// IdBundle ----------------------------------------------------------------------------------------
IdBundle::IdBundle(bool serializeFeatureIdOnly) : m_serializeFeatureIdOnly(serializeFeatureIdOnly) {}

IdBundle::IdBundle(FeatureId featureId, OsmId osmId, bool serializeFeatureIdOnly)
  : m_featureId(featureId)
  , m_osmId(osmId)
  , m_serializeFeatureIdOnly(serializeFeatureIdOnly)
{}

bool IdBundle::operator<(IdBundle const & rhs) const
{
  CHECK_EQUAL(m_serializeFeatureIdOnly, rhs.m_serializeFeatureIdOnly, ());

  if (m_serializeFeatureIdOnly)
    return m_featureId < rhs.m_featureId;

  return std::tie(m_featureId, m_osmId) < std::tie(rhs.m_featureId, rhs.m_osmId);
}

bool IdBundle::operator==(IdBundle const & rhs) const
{
  CHECK_EQUAL(m_serializeFeatureIdOnly, rhs.m_serializeFeatureIdOnly, ());
  return m_serializeFeatureIdOnly ? m_featureId == rhs.m_featureId
                                  : m_featureId == rhs.m_featureId && m_osmId == rhs.m_osmId;
}

bool IdBundle::operator!=(IdBundle const & rhs) const
{
  return !(*this == rhs);
}

bool IdBundle::IsValid() const
{
  return m_serializeFeatureIdOnly ? m_featureId != kInvalidFeatureId
                                  : m_featureId != kInvalidFeatureId && m_osmId != kInvalidOsmId;
}

void IdBundle::SetOsmId(OsmId osmId)
{
  m_osmId = osmId;
}

void IdBundle::SetFeatureId(FeatureId featureId)
{
  m_featureId = featureId;
}

OsmId IdBundle::GetOsmId() const
{
  return m_osmId;
}

FeatureId IdBundle::GetFeatureId() const
{
  return m_featureId;
}

bool IdBundle::SerializeFeatureIdOnly() const
{
  return m_serializeFeatureIdOnly;
}

// Network -----------------------------------------------------------------------------------------
Network::Network(TransitId id, std::string const & title) : m_id(id), m_title(title) {}

Network::Network(TransitId id) : m_id(id), m_title{} {}

bool Network::operator<(Network const & rhs) const
{
  return m_id < rhs.m_id;
}

bool Network::operator==(Network const & rhs) const
{
  return m_id == rhs.m_id;
}

bool Network::IsValid() const
{
  return m_id != kInvalidTransitId;
}

TransitId Network::GetId() const
{
  return m_id;
}

std::string const & Network::GetTitle() const
{
  return m_title;
}

// Route -------------------------------------------------------------------------------------------
Route::Route(TransitId id, TransitId networkId, std::string const & routeType, std::string const & title,
             std::string const & color)
  : m_id(id)
  , m_networkId(networkId)
  , m_routeType(routeType)
  , m_title(title)
  , m_color(color)
{}

bool Route::operator<(Route const & rhs) const
{
  return m_id < rhs.m_id;
}

bool Route::operator==(Route const & rhs) const
{
  return m_id == rhs.m_id;
}

bool Route::IsValid() const
{
  return m_id != kInvalidTransitId && m_networkId != kInvalidTransitId && !m_routeType.empty();
}

TransitId Route::GetId() const
{
  return m_id;
}

std::string const & Route::GetTitle() const
{
  return m_title;
}

std::string const & Route::GetType() const
{
  return m_routeType;
}

std::string const & Route::GetColor() const
{
  return m_color;
}

TransitId Route::GetNetworkId() const
{
  return m_networkId;
}

// Line --------------------------------------------------------------------------------------------
Line::Line(TransitId id, TransitId routeId, ShapeLink const & shapeLink, std::string const & title,
           IdList const & stopIds, Schedule const & schedule)
  : m_id(id)
  , m_routeId(routeId)
  , m_shapeLink(shapeLink)
  , m_title(title)
  , m_stopIds(stopIds)
  , m_schedule(schedule)
{}

bool Line::operator<(Line const & rhs) const
{
  return m_id < rhs.m_id;
}

bool Line::operator==(Line const & rhs) const
{
  return m_id == rhs.m_id;
}

bool Line::IsValid() const
{
  return m_id != kInvalidTransitId && m_routeId != kInvalidTransitId && m_shapeLink.m_shapeId != kInvalidTransitId &&
         !m_stopIds.empty();
}

TransitId Line::GetId() const
{
  return m_id;
}

std::string const & Line::GetTitle() const
{
  return m_title;
}

TransitId Line::GetRouteId() const
{
  return m_routeId;
}

ShapeLink const & Line::GetShapeLink() const
{
  return m_shapeLink;
}

IdList const & Line::GetStopIds() const
{
  return m_stopIds;
}

Schedule const & Line::GetSchedule() const
{
  return m_schedule;
}

// LineMetadata ------------------------------------------------------------------------------------
LineMetadata::LineMetadata(TransitId id, LineSegmentsOrder const & segmentsOrder)
  : m_id(id)
  , m_segmentsOrder(segmentsOrder)
{}

bool LineMetadata::operator<(LineMetadata const & rhs) const
{
  return m_id < rhs.GetId();
}
bool LineMetadata::operator==(LineMetadata const & rhs) const
{
  return m_id == rhs.GetId();
}

bool LineMetadata::IsValid() const
{
  return m_id != kInvalidTransitId;
}

TransitId LineMetadata::GetId() const
{
  return m_id;
}
LineSegmentsOrder const & LineMetadata::GetLineSegmentsOrder() const
{
  return m_segmentsOrder;
}

// Stop --------------------------------------------------------------------------------------------
Stop::Stop() : m_ids(true /* serializeFeatureIdOnly */) {}

Stop::Stop(TransitId id, FeatureId featureId, OsmId osmId, std::string const & title, TimeTable const & timetable,
           m2::PointD const & point, IdList const & transferIds)
  : m_id(id)
  , m_ids(featureId, osmId, true /* serializeFeatureIdOnly */)
  , m_title(title)
  , m_timetable(timetable)
  , m_point(point)
  , m_transferIds(transferIds)
{}

Stop::Stop(TransitId id) : m_id(id), m_ids(true /* serializeFeatureIdOnly */) {}

bool Stop::operator<(Stop const & rhs) const
{
  if (m_id != kInvalidTransitId || rhs.m_id != kInvalidTransitId)
    return m_id < rhs.m_id;

  return m_ids.GetFeatureId() < rhs.m_ids.GetFeatureId();
}

bool Stop::operator==(Stop const & rhs) const
{
  if (m_id != kInvalidTransitId || rhs.m_id != kInvalidTransitId)
    return m_id == rhs.m_id;

  return m_ids.GetFeatureId() == rhs.m_ids.GetFeatureId() && m_ids.GetOsmId() == rhs.m_ids.GetOsmId();
}

bool Stop::IsValid() const
{
  return ((m_id != kInvalidTransitId) || (m_ids.GetOsmId() != kInvalidOsmId) ||
          (m_ids.GetFeatureId() != kInvalidFeatureId));
}

FeatureId Stop::GetId() const
{
  return m_id;
}

FeatureId Stop::GetFeatureId() const
{
  return m_ids.GetFeatureId();
}

OsmId Stop::GetOsmId() const
{
  return m_ids.GetOsmId();
}

std::string const & Stop::GetTitle() const
{
  return m_title;
}

TimeTable const & Stop::GetTimeTable() const
{
  return m_timetable;
}

m2::PointD const & Stop::GetPoint() const
{
  return m_point;
}

IdList const & Stop::GetTransferIds() const
{
  return m_transferIds;
}

void Stop::SetBestPedestrianSegments(std::vector<SingleMwmSegment> const & seg)
{
  m_bestPedestrianSegments = seg;
}

std::vector<SingleMwmSegment> const & Stop::GetBestPedestrianSegments() const
{
  return m_bestPedestrianSegments;
}

// Gate --------------------------------------------------------------------------------------------
Gate::Gate() : m_ids(false /* serializeFeatureIdOnly */) {}

Gate::Gate(TransitId id, FeatureId featureId, OsmId osmId, bool entrance, bool exit,
           std::vector<TimeFromGateToStop> const & weights, m2::PointD const & point)
  : m_id(id)
  , m_ids(featureId, osmId, false /* serializeFeatureIdOnly */)
  , m_entrance(entrance)
  , m_exit(exit)
  , m_weights(weights)
  , m_point(point)
{}

bool Gate::operator<(Gate const & rhs) const
{
  return std::tie(m_id, m_ids, m_entrance, m_exit) < std::tie(rhs.m_id, rhs.m_ids, rhs.m_entrance, rhs.m_exit);
}

bool Gate::operator==(Gate const & rhs) const
{
  return std::tie(m_id, m_ids, m_entrance, m_exit) == std::tie(rhs.m_id, rhs.m_ids, rhs.m_entrance, rhs.m_exit);
}

bool Gate::IsValid() const
{
  return ((m_id != kInvalidTransitId) || (m_ids.GetOsmId() != kInvalidOsmId)) && (m_entrance || m_exit) &&
         !m_weights.empty();
}

void Gate::SetBestPedestrianSegments(std::vector<SingleMwmSegment> const & seg)
{
  m_bestPedestrianSegments = seg;
}

FeatureId Gate::GetFeatureId() const
{
  return m_ids.GetFeatureId();
}

OsmId Gate::GetOsmId() const
{
  return m_ids.GetOsmId();
}

TransitId Gate::GetId() const
{
  return m_id;
}

std::vector<SingleMwmSegment> const & Gate::GetBestPedestrianSegments() const
{
  return m_bestPedestrianSegments;
}

bool Gate::IsEntrance() const
{
  return m_entrance;
}

bool Gate::IsExit() const
{
  return m_exit;
}

std::vector<TimeFromGateToStop> const & Gate::GetStopsWithWeight() const
{
  return m_weights;
}

m2::PointD const & Gate::GetPoint() const
{
  return m_point;
}

// Edge --------------------------------------------------------------------------------------------
Edge::Edge(TransitId stop1Id, TransitId stop2Id, EdgeWeight weight, TransitId lineId, bool transfer,
           ShapeLink const & shapeLink)
  : m_stop1Id(stop1Id)
  , m_stop2Id(stop2Id)
  , m_weight(weight)
  , m_isTransfer(transfer)
  , m_lineId(lineId)
  , m_shapeLink(shapeLink)
{}

bool Edge::operator<(Edge const & rhs) const
{
  return std::tie(m_stop1Id, m_stop2Id, m_lineId) < std::tie(rhs.m_stop1Id, rhs.m_stop2Id, rhs.m_lineId);
}

bool Edge::operator==(Edge const & rhs) const
{
  return std::tie(m_stop1Id, m_stop2Id, m_lineId) == std::tie(rhs.m_stop1Id, rhs.m_stop2Id, rhs.m_lineId);
}

bool Edge::operator!=(Edge const & rhs) const
{
  return !(*this == rhs);
}

bool Edge::IsValid() const
{
  if (m_isTransfer && (m_lineId != kInvalidTransitId || m_shapeLink.m_shapeId != kInvalidTransitId))
    return false;

  if (!m_isTransfer && m_lineId == kInvalidTransitId)
    return false;

  return m_stop1Id != kInvalidTransitId && m_stop2Id != kInvalidTransitId;
}

void Edge::SetWeight(EdgeWeight weight)
{
  m_weight = weight;
}

TransitId Edge::GetStop1Id() const
{
  return m_stop1Id;
}

TransitId Edge::GetStop2Id() const
{
  return m_stop2Id;
}

EdgeWeight Edge::GetWeight() const
{
  return m_weight;
}

TransitId Edge::GetLineId() const
{
  return m_lineId;
}

bool Edge::IsTransfer() const
{
  return m_isTransfer;
}

ShapeLink const & Edge::GetShapeLink() const
{
  return m_shapeLink;
}

// Transfer ----------------------------------------------------------------------------------------
Transfer::Transfer(TransitId id, m2::PointD const & point, IdList const & stopIds)
  : m_id(id)
  , m_point(point)
  , m_stopIds(stopIds)
{}

bool Transfer::operator<(Transfer const & rhs) const
{
  return m_id < rhs.m_id;
}

bool Transfer::operator==(Transfer const & rhs) const
{
  return m_id == rhs.m_id;
}

bool Transfer::IsValid() const
{
  return m_id != kInvalidTransitId && m_stopIds.size() > 1;
}

TransitId Transfer::GetId() const
{
  return m_id;
}

m2::PointD const & Transfer::GetPoint() const
{
  return m_point;
}

IdList const & Transfer::GetStopIds() const
{
  return m_stopIds;
}

// Shape -------------------------------------------------------------------------------------------
Shape::Shape(TransitId id, std::vector<m2::PointD> const & polyline) : m_id(id), m_polyline(polyline) {}

bool Shape::operator<(Shape const & rhs) const
{
  return m_id < rhs.m_id;
}

bool Shape::operator==(Shape const & rhs) const
{
  return m_id == rhs.m_id;
}

bool Shape::IsValid() const
{
  return m_id != kInvalidTransitId && m_polyline.size() > 1;
}

TransitId Shape::GetId() const
{
  return m_id;
}

std::vector<m2::PointD> const & Shape::GetPolyline() const
{
  return m_polyline;
}
}  // namespace experimental
}  // namespace transit

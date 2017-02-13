#include "openlr/openlr_simple_parser.hpp"
#include "openlr/openlr_model.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include "std/cstring.hpp"
#include "std/type_traits.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace  // Primitive utilities to handle simple OpenLR-like XML data.
{
template <typename Value>
bool ParseInteger(pugi::xml_node const & node, Value & value)
{
  if (!node)
    return false;
  value = static_cast<Value>(is_signed<Value>::value
                             ? node.text().as_int()
                             : node.text().as_uint());
  return true;
}

bool GetLatLon(pugi::xml_node const & node, int32_t & lat, int32_t & lon)
{
  if (!ParseInteger(node.child("olr:latitude"), lat) ||
      !ParseInteger(node.child("olr:longitude"), lon))
  {
    return false;
  }

  return true;
}

// This helper is used to parse records like this:
// <olr:lfrcnp olr:table="olr001_FunctionalRoadClass" olr:code="4"/>
template <typename Value>
bool ParseTableValue(pugi::xml_node const & node, Value & value)
{
  if (!node)
    return false;
  value = static_cast<Value>(is_signed<Value>::value
                             ? node.attribute("olr:code").as_int()
                             : node.attribute("olr:code").as_uint());
  return true;
}

pugi::xml_node GetLinearLocationReference(pugi::xml_node const & node)
{
  // There should be only one linear location reference child of a location reference.
  return node.select_node(".//olr:locationReference/olr:optionLinearLocationReference").node();
}

// This helper is used do deal with xml nodes of the form
// <node>
//   <value>integer<value>
// </node>
template <typename Value>
bool ParseValue(pugi::xml_node const & node, Value & value)
{
  auto const valueNode = node.child("olr:value");
  return ParseInteger(valueNode, value);
}

template <typename Value>
bool ParseValueIfExists(pugi::xml_node const & node, Value & value)
{
  if (!node)
    return true;
  return ParseValue(node, value);
}
}  // namespace

namespace  // OpenLR tools and abstractions
{
bool GetFirstCoordinate(pugi::xml_node const & node, ms::LatLon & latLon)
{
  int32_t lat, lon;
  if (!GetLatLon(node.child("olr:coordinate"), lat, lon))
    return false;

  latLon.lat = ((lat - my::Sign(lat) * 0.5) * 360) / (1 << 24);
  latLon.lon = ((lon - my::Sign(lon) * 0.5) * 360) / (1 << 24);

  return true;
}

bool GetCoordinate(pugi::xml_node const & node, ms::LatLon const & firstCoord, ms::LatLon & latLon)
{
  int32_t lat, lon;
  if (!GetLatLon(node.child("olr:coordinate"), lat, lon))
    return false;

  latLon.lat = firstCoord.lat + static_cast<double>(lat) / 100000;
  latLon.lon = firstCoord.lon + static_cast<double>(lon) / 100000;

  return true;
}

bool ParseLineProperties(pugi::xml_node const & linePropNode,
                         openlr::LocationReferencePoint & locPoint)
{
  if (!linePropNode)
  {
    LOG(LERROR, ("linePropNode is NULL"));
    return false;
  }

  if (!ParseTableValue(linePropNode.child("olr:frc"), locPoint.m_functionalRoadClass))
  {
    LOG(LERROR, ("Can't parse functional road class"));
    return false;
  }

  if (!ParseTableValue(linePropNode.child("olr:fow"), locPoint.m_formOfWay))
  {
    LOG(LERROR, ("Can't parse form of a way"));
    return false;
  }

  if (!ParseValue(linePropNode.child("olr:bearing"), locPoint.m_bearing))
  {
    LOG(LERROR, ("Can't parse bearing"));
    return false;
  }

  return true;
}

bool ParsePathProperties(pugi::xml_node const & locPointNode,
                         openlr::LocationReferencePoint & locPoint)
{
  // Last point does not contain path properties.
  if (strcmp(locPointNode.name(), "olr:last") == 0)
    return true;

  auto const propNode = locPointNode.child("olr:pathProperties");
  if (!propNode)
  {
    LOG(LERROR, ("Can't parse path properties"));
    return false;
  }

  if (!ParseValue(propNode.child("olr:dnp"), locPoint.m_distanceToNextPoint))
  {
    LOG(LERROR, ("Can't parse dnp"));
    return false;
  }

  if (!ParseTableValue(propNode.child("olr:lfrcnp"), locPoint.m_lfrcnp))
  {
    LOG(LERROR, ("Can't parse lfrcnp"));
    return false;
  }

  auto const directionNode = propNode.child("olr:againstDrivingDirection");
  if (!directionNode)
  {
    LOG(LERROR, ("Can't parse driving direction"));
    return false;
  }
  locPoint.m_againstDrivingDirection = directionNode.text().as_bool();

  return true;
}

bool ParseLocationReferencePoint(pugi::xml_node const & locPointNode,
                                 openlr::LocationReferencePoint & locPoint)
{
  if (!GetFirstCoordinate(locPointNode, locPoint.m_latLon))
  {
    LOG(LERROR, ("Can't get first coordinate"));
    return false;
  }

  return ParseLineProperties(locPointNode.child("olr:lineProperties"), locPoint) &&
      ParsePathProperties(locPointNode, locPoint);
}

bool ParseLocationReferencePoint(pugi::xml_node const & locPointNode, ms::LatLon const & firstPoint,
                                 openlr::LocationReferencePoint & locPoint)
{
  if (!GetCoordinate(locPointNode, firstPoint, locPoint.m_latLon))
  {
    LOG(LERROR, ("Can't get last coordinate"));
    return false;
  }

  return ParseLineProperties(locPointNode.child("olr:lineProperties"), locPoint) &&
      ParsePathProperties(locPointNode, locPoint);
}

bool ParseLinearLocationReference(pugi::xml_node const & locRefNode,
                                  openlr::LinearLocationReference & locRef)
{
  if (!locRefNode)
  {
    LOG(LERROR, ("Can't get loaction reference"));
    return false;
  }

  {
    openlr::LocationReferencePoint point;
    if (!ParseLocationReferencePoint(locRefNode.child("olr:first"), point))
      return false;
    locRef.m_points.push_back(point);
  }

  for (auto const pointNode : locRefNode.select_nodes("olr:intermediates"))
  {
    openlr::LocationReferencePoint point;
    if (!ParseLocationReferencePoint(pointNode.node(), locRef.m_points.back().m_latLon, point))
      return false;
    locRef.m_points.push_back(point);
  }

  {
    openlr::LocationReferencePoint point;
    if (!ParseLocationReferencePoint(locRefNode.child("olr:last"), locRef.m_points.back().m_latLon,
                                     point))
      return false;
    locRef.m_points.push_back(point);
  }

  if (!ParseValueIfExists(locRefNode.child("olr:positiveOffset"), locRef.m_positiveOffsetMeters))
  {
    LOG(LERROR, ("Can't parse positive offset"));
    return false;
  }

  if(!ParseValueIfExists(locRefNode.child("olr:negativeOffset"), locRef.m_negativeOffsetMeters))
  {
    LOG(LERROR, ("Can't parse negative offset"));
    return false;
  }

  return true;
}

bool ParseSegment(pugi::xml_node const & segmentNode, openlr::LinearSegment & segment)
{
  if (!ParseInteger(segmentNode.child("ReportSegmentID"), segment.m_segmentId))
  {
    LOG(LERROR, ("Can't parse segment id"));
    return false;
  }

  if (!ParseInteger(segmentNode.child("segmentLength"), segment.m_segmentLengthMeters))
  {
    LOG(LERROR, ("Can't parse segment length"));
    return false;
  }

  auto const locRefNode = GetLinearLocationReference(segmentNode);
  return ParseLinearLocationReference(locRefNode, segment.m_locationReference);
}
}  // namespace

namespace openlr
{
// Functions ---------------------------------------------------------------------------------------
bool ParseOpenlr(pugi::xml_document const & document, vector<LinearSegment> & segments)
{
  for (auto const segmentXpathNode : document.select_nodes("//reportSegments"))
  {
    LinearSegment segment;
    if (!ParseSegment(segmentXpathNode.node(), segment))
      return false;
    segments.push_back(segment);
  }
  return true;
}
}  // namespace openlr

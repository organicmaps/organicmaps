#pragma once

#include "routing/road_graph.hpp"

#include "base/exception.hpp"
#include "base/newtype.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <pugixml.hpp>

class DataSource;

namespace openlr
{
using Path = routing::RoadGraphBase::EdgeVector;
using routing::Edge;

NEWTYPE(uint32_t, PartnerSegmentId);
NEWTYPE_SIMPLE_OUTPUT(PartnerSegmentId);

DECLARE_EXCEPTION(DecodedPathLoadError, RootException);
DECLARE_EXCEPTION(DecodedPathSaveError, RootException);

struct DecodedPath
{
  PartnerSegmentId m_segmentId;
  Path m_path;
};

uint32_t UintValueFromXML(pugi::xml_node const & node);

void WriteAsMappingForSpark(std::string const & fileName, std::vector<DecodedPath> const & paths);
void WriteAsMappingForSpark(std::ostream & ost, std::vector<DecodedPath> const & paths);

void PathFromXML(pugi::xml_node const & node, DataSource const & dataSource, Path & path);
void PathToXML(Path const & path, pugi::xml_node & node);
}  // namespace openlr

namespace routing
{
inline m2::PointD GetStart(Edge const & e)
{
  return e.GetStartJunction().GetPoint();
}
inline m2::PointD GetEnd(Edge const & e)
{
  return e.GetEndJunction().GetPoint();
}

std::vector<m2::PointD> GetPoints(routing::RoadGraphBase::EdgeVector const & p);
}  // namespace routing

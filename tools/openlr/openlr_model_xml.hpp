#pragma once

#include <vector>

namespace pugi
{
class xml_document;
class xml_node;
}  // namespace pugi

namespace openlr
{
struct LinearSegment;

bool SegmentFromXML(pugi::xml_node const & segmentNode, LinearSegment & segment);

bool ParseOpenlr(pugi::xml_document const & document, std::vector<LinearSegment> & segments);
}  // namespace openlr

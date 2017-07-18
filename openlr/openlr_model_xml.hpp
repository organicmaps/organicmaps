#pragma once

#include "std/vector.hpp"

namespace pugi
{
class xml_document;
class xml_node;
}  // namespace pugi

namespace openlr
{
struct LinearSegment;

bool SegmentFromXML(pugi::xml_node const & segmentNode, LinearSegment & segment);

bool ParseOpenlr(pugi::xml_document const & document, vector<LinearSegment> & segments);
}  // namespace openlr

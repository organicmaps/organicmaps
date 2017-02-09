#pragma once

#include "std/vector.hpp"

namespace pugi
{
class xml_document;
}  // namespace pugi

namespace openlr
{
struct LinearSegment;

bool ParseOpenlr(pugi::xml_document const & document, vector<LinearSegment> & segments);
}  // namespace openlr

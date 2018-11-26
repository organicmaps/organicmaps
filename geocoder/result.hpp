#pragma once

#include "base/geo_object_id.hpp"

#include <string>

namespace geocoder
{
struct Result
{
  Result() = default;

  Result(base::GeoObjectId const & osmId, double certainty) : m_osmId(osmId), m_certainty(certainty)
  {
  }

  // The encoded osm id of a node, way or relation.
  base::GeoObjectId m_osmId = base::GeoObjectId(base::GeoObjectId::kInvalid);

  // A floating point number in the range [0.0, 1.0]
  // describing the extent to which the result matches
  // the query.
  // 0.0 corresponds to the least probable results and
  // 1.0 to the most probable.
  double m_certainty = 0;
};

std::string DebugPrint(Result const & result);
}  // namespace geocoder

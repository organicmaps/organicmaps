#include "generator/filter_roads.hpp"

namespace generator
{
std::shared_ptr<FilterInterface> FilterRoads::Clone() const
{
  return std::make_shared<FilterRoads>();
}

bool FilterRoads::IsAccepted(OsmElement const & element) const
{
  return !element.HasTag("highway", "ice_road");
}
}  // namespace generator

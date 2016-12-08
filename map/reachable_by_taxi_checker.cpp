#include "map/reachable_by_taxi_checker.hpp"

#include "indexer/classificator.hpp"

namespace place_page
{
IsReachableByTaxiChecker::IsReachableByTaxiChecker()
{
  // Returns true when the following list does not contain provided type.
  Classificator const & c = classif();
  char const * const paths[][2] = {
    {"aerialway", "cable_car"},
    {"aerialway", "chair_lift"},
    {"aerialway", "drag_lift"},
    {"aerialway", "gondola"},
    {"aerialway", "mixed_lift"},
    {"aeroway", "gate"},
    {"amenity", "bench"},
    {"amenity", "fountain"},
    {"amenity", "grave_yard"},
    {"amenity", "hunting_stand"},
    {"amenity", "post_box"},
    {"amenity", "recycling"},
    {"amenity", "toilets"},
    {"amenity", "waste_basket"},
    {"amenity", "waste_disposal"},
    {"barrier", "border_control"},
    {"barrier", "lift_gate"},
    {"barrier", "toll_booth"},
    {"emergency", "phone"},
    {"highway", "bus_stop"},
    {"highway", "motorway_junction"},
    {"highway", "speed_camera"},
    {"historic", "wayside_cross"},
    {"historic", "wayside_shrine"},
    {"landuse", "allotments"},
    {"landuse", "cemetery"},
    {"landuse", "construction"},
    {"landuse", "farmland"},
    {"landuse", "forest"},
    {"landuse", "industrial"},
    {"landuse", "landfill"},
    {"landuse", "military"},
    {"landuse", "orchard"},
    {"landuse", "quarry"},
    {"landuse", "railway"},
    {"landuse", "recreation_ground"},
    {"landuse", "vineyard"},
    {"man_made", "breakwater"},
    {"man_made", "chimney"},
    {"man_made", "lighthouse"},
    {"man_made", "pier"},
    {"man_made", "windmill"},
    {"natural", "bare_rock"},
    {"natural", "cape"},
    {"natural", "cave_entrance"},
    {"natural", "peak"},
    {"natural", "rock"},
    {"natural", "spring"},
    {"natural", "tree"},
    {"natural", "volcano"},
    {"natural", "water"},
    {"natural", "wetland"},
    {"piste:lift", "j-bar"},
    {"piste:lift", "magic_carpet"},
    {"piste:lift", "platter"},
    {"piste:lift", "rope_tow"},
    {"piste:lift", "t-bar"},
    {"place", "city"},
    {"place", "continent"},
    {"place", "country"},
    {"place", "farm"},
    {"place", "hamlet"},
    {"place", "island"},
    {"place", "isolated_dwelling"},
    {"place", "locality"},
    {"place", "neighbourhood"},
    {"place", "ocean"},
    {"place", "region"},
    {"place", "sea"},
    {"place", "state"},
    {"place", "suburb"},
    {"place", "town"},
    {"place", "village"},
    {"waterway", "lock_gate"},
    {"waterway", "waterfall"}
  };

  for (auto const & path : paths)
    m_types.push_back(c.GetTypeByPath({path[0], path[1]}));
}

bool IsReachableByTaxiChecker::IsMatched(uint32_t type) const
{
  return !BaseChecker::IsMatched(type);
}

IsReachableByTaxiChecker const & IsReachableByTaxiChecker::Instance()
{
  static IsReachableByTaxiChecker const inst;
  return inst;
}
}  // namespace place_page

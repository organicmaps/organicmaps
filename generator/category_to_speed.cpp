#include "generator/category_to_speed.hpp"

#include <unordered_map>

namespace
{
using namespace measurement_utils;

struct Speed
{
  Speed() = delete;

  uint16_t m_speed = 0;
  Units m_units = Units::Metric;
};

std::unordered_map<std::string, Speed> const kRoadCategoryToSpeedKMpH = {
    {"AT:urban", {50, Units::Metric}},
    {"AT:rural", {100, Units::Metric}},
    {"AT:trunk", {100, Units::Metric}},
    {"AT:motorway", {130, Units::Metric}},
    {"BE:urban", {50, Units::Metric}},
    {"BE:zone", {30, Units::Metric}},
    {"BE:motorway", {120, Units::Metric}},
    {"BE:zone30", {30, Units::Metric}},
    {"BE:rural", {70, Units::Metric}},
    {"BE:school", {30, Units::Metric}},
    {"CZ:motorway", {130, Units::Metric}},
    {"CZ:trunk", {110, Units::Metric}},
    {"CZ:rural", {90, Units::Metric}},
    {"CZ:urban_motorway", {80, Units::Metric}},
    {"CZ:urban_trunk", {80, Units::Metric}},
    {"CZ:urban", {50, Units::Metric}},
    {"DE:rural", {100, Units::Metric}},
    {"DE:urban", {50, Units::Metric}},
    {"DE:bicycle_road", {30, Units::Metric}},
    {"FR:motorway", {130, Units::Metric}},
    {"FR:rural", {80, Units::Metric}},
    {"FR:urban", {50, Units::Metric}},
    {"HU:living_street", {20, Units::Metric}},
    {"HU:motorway", {130, Units::Metric}},
    {"HU:rural", {90, Units::Metric}},
    {"HU:trunk", {110, Units::Metric}},
    {"HU:urban", {50, Units::Metric}},
    {"IT:rural", {90, Units::Metric}},
    {"IT:motorway", {130, Units::Metric}},
    {"IT:urban", {50, Units::Metric}},
    {"JP:nsl", {60, Units::Metric}},
    {"JP:express", {100, Units::Metric}},
    {"LT:rural", {90, Units::Metric}},
    {"LT:urban", {50, Units::Metric}},
    {"NO:rural", {80, Units::Metric}},
    {"NO:urban", {50, Units::Metric}},
    {"ON:urban", {50, Units::Metric}},
    {"ON:rural", {80, Units::Metric}},
    {"PT:motorway", {120, Units::Metric}},
    {"PT:rural", {90, Units::Metric}},
    {"PT:trunk", {100, Units::Metric}},
    {"PT:urban", {50, Units::Metric}},
    {"RO:motorway", {130, Units::Metric}},
    {"RO:rural", {90, Units::Metric}},
    {"RO:trunk", {100, Units::Metric}},
    {"RO:urban", {50, Units::Metric}},
    {"RU:living_street", {20, Units::Metric}},
    {"RU:urban", {60, Units::Metric}},
    {"RU:rural", {90, Units::Metric}},
    {"RU:motorway", {110, Units::Metric}},

    {"GB:motorway", {70, Units::Imperial}},  // 70 mph = 112.65408 kmph
    {"GB:nsl_dual", {70, Units::Imperial}},  // 70 mph = 112.65408 kmph
    {"GB:nsl_single", {60, Units::Imperial}},   // 60 mph = 96.56064 kmph

    {"UK:motorway", {70, Units::Imperial}},  // 70 mph
    {"UK:nsl_dual", {70, Units::Imperial}},  // 70 mph
    {"UK:nsl_single", {60, Units::Imperial}},   // 60 mph

    {"UZ:living_street", {30, Units::Metric}},
    {"UZ:urban", {70, Units::Metric}},
    {"UZ:rural", {100, Units::Metric}},
    {"UZ:motorway", {110, Units::Metric}},
};
}  // namespace

namespace generator
{
bool RoadCategoryToSpeed(std::string const & category, double & speed,
                         measurement_utils::Units & units)
{
  auto const it = kRoadCategoryToSpeedKMpH.find(category);
  if (it == kRoadCategoryToSpeedKMpH.cend())
    return false;

  speed = it->second.m_speed;
  units = it->second.m_units;
  return true;
}
}  // generator

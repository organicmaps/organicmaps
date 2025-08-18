#include "generator/maxspeeds_parser.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <cctype>
#include <limits>
#include <unordered_map>

namespace generator
{
using measurement_utils::Units;

static std::unordered_map<std::string, routing::SpeedInUnits> const kRoadCategoryToSpeed = {
    {"AR:urban", {40, Units::Metric}},
    {"AR:urban:primary", {60, Units::Metric}},
    {"AR:urban:secondary", {60, Units::Metric}},
    {"AR:rural", {110, Units::Metric}},
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
    {"CH:urban", {50, Units::Metric}},
    {"CH:rural", {80, Units::Metric}},
    {"CH:trunk", {100, Units::Metric}},
    {"CH:motorway", {120, Units::Metric}},
    {"CZ:pedestrian_zone", {20, Units::Metric}},
    {"CZ:living_street", {20, Units::Metric}},
    {"CZ:urban", {50, Units::Metric}},
    {"CZ:urban_trunk", {80, Units::Metric}},
    {"CZ:urban_motorway", {80, Units::Metric}},
    {"CZ:rural", {90, Units::Metric}},
    {"CZ:trunk", {110, Units::Metric}},
    {"CZ:motorway", {130, Units::Metric}},
    {"DK:urban", {50, Units::Metric}},
    {"DK:rural", {80, Units::Metric}},
    {"DK:motorway", {130, Units::Metric}},
    {"DE:living_street", {7, Units::Metric}},
    {"DE:urban", {50, Units::Metric}},
    {"DE:rural", {100, Units::Metric}},
    {"DE:bicycle_road", {30, Units::Metric}},
    {"DE:trunk", {routing::kNoneMaxSpeed, Units::Metric}},
    {"DE:motorway", {routing::kNoneMaxSpeed, Units::Metric}},
    {"FI:urban", {50, Units::Metric}},
    {"FI:rural", {80, Units::Metric}},
    {"FI:trunk", {100, Units::Metric}},
    {"FI:motorway", {120, Units::Metric}},
    {"FR:motorway", {130, Units::Metric}},
    {"FR:trunk", {110, Units::Metric}},
    {"FR:rural", {80, Units::Metric}},
    {"FR:urban", {50, Units::Metric}},
    {"GR:urban", {50, Units::Metric}},
    {"GR:rural", {90, Units::Metric}},
    {"GR:trunk", {90, Units::Metric}},
    {"GR:motorway", {130, Units::Metric}},
    {"HU:living_street", {20, Units::Metric}},
    {"HU:motorway", {130, Units::Metric}},
    {"HU:rural", {90, Units::Metric}},
    {"HU:trunk", {110, Units::Metric}},
    {"HU:urban", {50, Units::Metric}},
    {"IT:motorway", {130, Units::Metric}},
    {"IT:trunk", {110, Units::Metric}},
    {"IT:rural", {90, Units::Metric}},
    {"IT:urban", {50, Units::Metric}},
    {"JP:national", {60, Units::Metric}},
    {"JP:motorway", {100, Units::Metric}},
    {"JP:nsl", {60, Units::Metric}},
    {"JP:express", {100, Units::Metric}},
    {"LT:motorway", {110, Units::Metric}},
    {"LT:trunk", {110, Units::Metric}},
    {"LT:rural", {90, Units::Metric}},
    {"LT:urban", {50, Units::Metric}},
    {"LT:living_street", {20, Units::Metric}},
    {"NO:rural", {80, Units::Metric}},
    {"NO:urban", {50, Units::Metric}},
    {"ON:urban", {50, Units::Metric}},
    {"ON:rural", {80, Units::Metric}},
    {"PL:living_street", {20, Units::Metric}},
    {"PL:urban", {50, Units::Metric}},
    {"PL:rural", {90, Units::Metric}},
    {"PL:trunk", {100, Units::Metric}},
    {"PL:motorway", {140, Units::Metric}},
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
    {"SK:urban", {50, Units::Metric}},
    {"SK:rural", {90, Units::Metric}},
    {"SK:trunk", {90, Units::Metric}},
    {"SK:motorway", {90, Units::Metric}},
    {"SL:urban", {50, Units::Metric}},
    {"SL:rural", {90, Units::Metric}},
    {"SL:trunk", {110, Units::Metric}},
    {"SL:motorway", {130, Units::Metric}},
    {"ES:urban", {50, Units::Metric}},
    {"ES:rural", {90, Units::Metric}},
    {"ES:trunk", {100, Units::Metric}},
    {"ES:motorway", {120, Units::Metric}},
    {"SE:urban", {50, Units::Metric}},
    {"SE:rural", {70, Units::Metric}},
    {"SE:trunk", {90, Units::Metric}},
    {"SE:motorway", {110, Units::Metric}},

    {"GB:motorway", {70, Units::Imperial}},    // 70 mph = 112.65408 kmph
    {"GB:nsl_dual", {70, Units::Imperial}},    // 70 mph = 112.65408 kmph
    {"GB:nsl_single", {60, Units::Imperial}},  // 60 mph = 96.56064 kmph

    {"UK:motorway", {70, Units::Imperial}},    // 70 mph
    {"UK:nsl_dual", {70, Units::Imperial}},    // 70 mph
    {"UK:nsl_single", {60, Units::Imperial}},  // 60 mph

    {"UA:urban", {50, Units::Metric}},
    {"UA:rural", {90, Units::Metric}},
    {"UA:trunk", {110, Units::Metric}},
    {"UA:motorway", {130, Units::Metric}},
    {"UZ:living_street", {30, Units::Metric}},
    {"UZ:urban", {70, Units::Metric}},
    {"UZ:rural", {100, Units::Metric}},
    {"UZ:motorway", {110, Units::Metric}},
};

bool RoadCategoryToSpeed(std::string const & category, routing::SpeedInUnits & speed)
{
  auto const it = kRoadCategoryToSpeed.find(category);
  if (it == kRoadCategoryToSpeed.cend())
    return false;

  speed = it->second;
  return true;
}

bool ParseMaxspeedTag(std::string const & maxspeedValue, routing::SpeedInUnits & speed)
{
  if (RoadCategoryToSpeed(maxspeedValue, speed))
    return true;

  if (maxspeedValue == "none")
  {
    speed.SetSpeed(routing::kNoneMaxSpeed);
    speed.SetUnits(Units::Metric);  // It's dummy value in case of kNoneMaxSpeed
    return true;
  }

  if (maxspeedValue == "walk")
  {
    speed.SetSpeed(routing::kWalkMaxSpeed);
    speed.SetUnits(Units::Metric);  // It's dummy value in case of kWalkMaxSpeed
    return true;
  }

  // strings::to_int doesn't work here because of bad errno.
  std::string speedStr;
  size_t i;
  for (i = 0; i < maxspeedValue.size(); ++i)
  {
    if (!isdigit(maxspeedValue[i]))
      break;

    speedStr += maxspeedValue[i];
  }

  while (i < maxspeedValue.size() && strings::IsASCIISpace(maxspeedValue[i]))
    ++i;

  if (maxspeedValue.size() == i || maxspeedValue.substr(i).starts_with("kmh"))
  {
    uint64_t kmph = 0;
    if (!strings::to_uint64(speedStr.c_str(), kmph) || kmph == 0 || kmph > std::numeric_limits<uint16_t>::max())
      return false;

    speed.SetSpeed(static_cast<uint16_t>(kmph));
    speed.SetUnits(Units::Metric);
    return true;
  }

  if (maxspeedValue.substr(i).starts_with("mph"))
  {
    uint64_t mph = 0;
    if (!strings::to_uint64(speedStr.c_str(), mph) || mph == 0 || mph > std::numeric_limits<uint16_t>::max())
      return false;

    speed.SetSpeed(static_cast<uint16_t>(mph));
    speed.SetUnits(Units::Imperial);
    return true;
  }

  return false;
}

std::string UnitsToString(Units units)
{
  switch (units)
  {
  case Units::Metric: return "Metric";
  case Units::Imperial: return "Imperial";
  }
  UNREACHABLE();
}

Units StringToUnits(std::string_view units)
{
  if (units == "Metric")
    return Units::Metric;
  if (units == "Imperial")
    return Units::Imperial;

  CHECK(false, (units));
  return Units::Metric;
}
}  // namespace generator

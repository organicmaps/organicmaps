#include "routing/router.hpp"

namespace routing
{
std::string ToString(RouterType type)
{
  switch(type)
  {
  case RouterType::Vehicle: return "Vehicle";
  case RouterType::Pedestrian: return "Pedestrian";
  case RouterType::Bicycle: return "Bicycle";
  case RouterType::Taxi: return "Taxi";
  }
  ASSERT(false, ());
  return "Error";
}

RouterType FromString(std::string const & str)
{
  if (str == "vehicle")
    return RouterType::Vehicle;
  if (str == "pedestrian")
    return RouterType::Pedestrian;
  if (str == "bicycle")
    return RouterType::Bicycle;
  if (str == "taxi")
    return RouterType::Taxi;

  ASSERT(false, ("Incorrect routing string:", str));
  return RouterType::Vehicle;
}
} //  namespace routing

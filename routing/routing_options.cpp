#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <sstream>

namespace routing
{
std::string const RoutingOptions::kAvoidRoutingOptionSettings = "avoid_routing_options";

// static
RoutingOptions RoutingOptions::LoadFromSettings()
{
  RoadType mode = 0;
  UNUSED_VALUE(settings::Get(kAvoidRoutingOptionSettings, mode));

  return RoutingOptions(mode);
}

void RoutingOptions::Add(RoutingOptions::Road type)
{
  m_options |= static_cast<RoadType>(type);
}

void RoutingOptions::Remove(RoutingOptions::Road type)
{
  m_options &= ~static_cast<RoadType>(type);
}

bool RoutingOptions::Has(RoutingOptions::Road type) const
{
  return (m_options & static_cast<RoadType>(type)) != 0;
}

std::string DebugPrint(RoutingOptions const & routingOptions)
{
  std::ostringstream ss;
  ss << "RoutingOptions: {";

  if (routingOptions.Has(RoutingOptions::Road::Usual))
    ss << " | " << DebugPrint(RoutingOptions::Road::Usual);

  if (routingOptions.Has(RoutingOptions::Road::Toll))
    ss << " | " << DebugPrint(RoutingOptions::Road::Toll);

  if (routingOptions.Has(RoutingOptions::Road::Motorway))
    ss << " | " << DebugPrint(RoutingOptions::Road::Motorway);

  if (routingOptions.Has(RoutingOptions::Road::Ferry))
    ss << " | " << DebugPrint(RoutingOptions::Road::Ferry);

  if (routingOptions.Has(RoutingOptions::Road::Dirty))
    ss << " | " << DebugPrint(RoutingOptions::Road::Dirty);

  ss << "}";
  return ss.str();
}
std::string DebugPrint(RoutingOptions::Road type)
{
  switch (type)
  {
  case RoutingOptions::Road::Toll: return "toll";
  case RoutingOptions::Road::Motorway: return "motorway";
  case RoutingOptions::Road::Ferry: return "ferry";
  case RoutingOptions::Road::Dirty: return "dirty";
  case RoutingOptions::Road::Usual: return "usual";
  }

  UNREACHABLE();
}
}  // namespace routing

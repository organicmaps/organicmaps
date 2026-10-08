#include "routing/routing_options.hpp"

#include "routing/router.hpp"

#include "platform/settings.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace routing
{
// RoutingOptions -------------------------------------------------------------------------------------

std::string_view constexpr kRouteOptimizationEnabledKey = "RouteOptimizationEnabled";

namespace
{
std::string_view SettingsKey(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car: return "avoid_routing_options_car";
  case VehicleType::Bicycle: return "avoid_routing_options_bicycle";
  case VehicleType::Pedestrian: return "avoid_routing_options_pedestrian";
  case VehicleType::Transit: return {};
  case VehicleType::Count: break;
  }
  CHECK(false, (vehicleType));
  return {};
}
}  // namespace

RoutingOptions::RoadType RoutingOptions::GetSupportedOptions(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car: return Toll | Motorway | Ferry | Dirty;
  case VehicleType::Bicycle:
  case VehicleType::Pedestrian: return Ferry;
  case VehicleType::Transit: return 0;
  case VehicleType::Count: break;
  }
  CHECK(false, (vehicleType));
  return 0;
}

std::optional<VehicleType> RoutingOptions::GetVehicleType(RouterType routerType)
{
  switch (routerType)
  {
  case RouterType::Vehicle: return VehicleType::Car;
  case RouterType::Bicycle: return VehicleType::Bicycle;
  case RouterType::Pedestrian: return VehicleType::Pedestrian;
  case RouterType::Transit: return VehicleType::Transit;
  case RouterType::Ruler: return {};
  case RouterType::Count: break;
  }
  CHECK(false, (routerType));
  return {};
}

std::optional<RoutingOptions::Road> RoutingOptions::RoadFromId(uint32_t id)
{
  switch (id)
  {
  case Usual:
  case Toll:
  case Motorway:
  case Ferry:
  case Dirty:
  case Steps: return static_cast<Road>(id);
  default: return {};
  }
}

// static
RoutingOptions RoutingOptions::LoadFromSettings(VehicleType vehicleType)
{
  auto const key = SettingsKey(vehicleType);
  uint32_t mask = 0;
  if (!key.empty())
    settings::TryGet(key, mask);
  return RoutingOptions(static_cast<RoadType>(mask & GetSupportedOptions(vehicleType)));
}

// static
void RoutingOptions::SaveToSettings(VehicleType vehicleType, RoutingOptions options)
{
  auto const key = SettingsKey(vehicleType);
  if (!key.empty())
    settings::Set(key, static_cast<uint32_t>(options.GetOptions() & GetSupportedOptions(vehicleType)));
}

// static
bool RoutingOptions::LoadRouteOptimizationFromSettings()
{
  return settings::IsEnabled(kRouteOptimizationEnabledKey);
}

// static
void RoutingOptions::SaveRouteOptimizationToSettings(bool enabled)
{
  settings::Set(kRouteOptimizationEnabledKey, enabled);
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

// RoutingOptionsClassifier ---------------------------------------------------------------------------

RoutingOptionsClassifier::RoutingOptionsClassifier()
{
  Classificator const & c = classif();

  std::pair<std::vector<std::string>, RoutingOptions::Road> const types[] = {
      {{"highway", "motorway"}, RoutingOptions::Road::Motorway},

      {{"hwtag", "toll"}, RoutingOptions::Road::Toll},

      {{"route", "ferry"}, RoutingOptions::Road::Ferry},

      {{"highway", "track"}, RoutingOptions::Road::Dirty},
      {{"highway", "road"}, RoutingOptions::Road::Dirty},
      {{"psurface", "unpaved_bad"}, RoutingOptions::Road::Dirty},
      {{"psurface", "unpaved_good"}, RoutingOptions::Road::Dirty},

      {{"highway", "steps"}, RoutingOptions::Road::Steps}};

  m_data.Reserve(std::size(types));
  for (auto const & data : types)
    m_data.Insert(c.GetTypeByPath(data.first), data.second);
  m_data.FinishBuilding();
}

std::optional<RoutingOptions::Road> RoutingOptionsClassifier::Get(uint32_t type) const
{
  ftype::TruncValue(type, 2);  // in case of highway-motorway-bridge

  auto const * res = m_data.Find(type);
  if (res)
    return *res;
  return {};
}

RoutingOptionsClassifier const & RoutingOptionsClassifier::Instance()
{
  static RoutingOptionsClassifier instance;
  return instance;
}

std::string DebugPrint(RoutingOptions const & routingOptions)
{
  std::ostringstream ss;
  ss << "RoutingOptions: {";

  bool wasAppended = false;
  auto const append = [&](RoutingOptions::Road road)
  {
    if (routingOptions.Has(road))
    {
      wasAppended = true;
      ss << " | " << DebugPrint(road);
    }
  };

  append(RoutingOptions::Road::Usual);
  append(RoutingOptions::Road::Toll);
  append(RoutingOptions::Road::Motorway);
  append(RoutingOptions::Road::Ferry);
  append(RoutingOptions::Road::Dirty);
  append(RoutingOptions::Road::Steps);

  if (wasAppended)
    ss << " | ";

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
  case RoutingOptions::Road::Steps: return "steps";
  case RoutingOptions::Road::Usual: return "usual";
  case RoutingOptions::Road::Max: return "max";
  }

  UNREACHABLE();
}

RoutingOptionSetter::RoutingOptionSetter(RoutingOptions::RoadType roadsMask, VehicleType vehicleType)
  : m_vehicleType(vehicleType)
{
  auto const key = SettingsKey(vehicleType);
  std::string value;
  if (!key.empty() && settings::Get(key, value))
    m_saved = value;
  RoutingOptions::SaveToSettings(vehicleType, RoutingOptions(roadsMask));
}

RoutingOptionSetter::~RoutingOptionSetter()
{
  auto const key = SettingsKey(m_vehicleType);
  if (key.empty())
    return;
  if (m_saved)
    settings::Set(key, *m_saved);
  else
    settings::Delete(key);
}

}  // namespace routing

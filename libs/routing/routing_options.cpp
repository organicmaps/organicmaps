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
std::string_view constexpr kBicycleSurfacePreferenceKey = "BicycleSurfacePreference";
std::string_view constexpr kTransitModesKey = "TransitAllowedModes";
std::string_view constexpr kTransitPreferenceKey = "TransitRoutePreference";

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
  case VehicleType::Bicycle: return Ferry | Dirty | Steps;
  case VehicleType::Pedestrian: return Ferry | Steps;
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

RoutePlanningOptions RoutePlanningOptions::LoadFromSettings(VehicleType vehicle)
{
  RoutePlanningOptions options{RoutingOptions::LoadFromSettings(vehicle)};
  if (vehicle == VehicleType::Transit)
  {
    options.m_transitModes = LoadTransitModes();
    options.m_transitPreference = LoadTransitPreference();
  }
  if (vehicle != VehicleType::Bicycle || options.m_avoid.Has(RoutingOptions::Dirty))
    return options;

  uint32_t value = 0;
  if (settings::Get(kBicycleSurfacePreferenceKey, value) &&
      value < static_cast<uint32_t>(BicycleSurfacePreference::AvoidUnpaved))
    options.m_bicycleSurface = static_cast<BicycleSurfaceBias>(value);
  return options;
}

BicycleSurfacePreference RoutePlanningOptions::LoadBicycleSurfacePreference()
{
  auto const options = LoadFromSettings(VehicleType::Bicycle);
  if (options.m_avoid.Has(RoutingOptions::Dirty))
    return BicycleSurfacePreference::AvoidUnpaved;
  return static_cast<BicycleSurfacePreference>(options.m_bicycleSurface);
}

void RoutePlanningOptions::SaveBicycleSurfacePreference(BicycleSurfacePreference preference)
{
  auto const value = static_cast<int64_t>(preference);
  CHECK(preference >= BicycleSurfacePreference::Any && preference <= BicycleSurfacePreference::AvoidUnpaved, (value));
  auto avoids = RoutingOptions::LoadFromSettings(VehicleType::Bicycle);
  // The place page also updates the Dirty bit for Avoid unpaved.
  if (preference == BicycleSurfacePreference::AvoidUnpaved)
    avoids.Add(RoutingOptions::Dirty);
  else
    avoids.Remove(RoutingOptions::Dirty);
  settings::Update({{std::string(kBicycleSurfacePreferenceKey), settings::ToString(value)},
                    {std::string(SettingsKey(VehicleType::Bicycle)),
                     settings::ToString(static_cast<uint32_t>(avoids.GetOptions()))}});
}

transit::ModeMask RoutePlanningOptions::LoadTransitModes()
{
  uint32_t value = 0;
  settings::TryGet(kTransitModesKey, value);
  auto const modes = static_cast<transit::ModeMask>(value & transit::kAllModes);
  return modes == 0 ? transit::kAllModes : modes;
}

void RoutePlanningOptions::SaveTransitModes(uint64_t modes)
{
  CHECK(modes != 0 && (modes & ~uint64_t{transit::kAllModes}) == 0, (modes));
  settings::Set(kTransitModesKey, static_cast<uint32_t>(modes));
}

TransitPreference LoadTransitPreference()
{
  uint32_t value = 0;
  if (settings::Get(kTransitPreferenceKey, value) && value <= static_cast<uint32_t>(TransitPreference::FewerTransfers))
    return static_cast<TransitPreference>(value);
  return TransitPreference::Recommended;
}

void SaveTransitPreference(TransitPreference preference)
{
  auto const value = static_cast<uint32_t>(preference);
  CHECK_LESS_OR_EQUAL(value, static_cast<uint32_t>(TransitPreference::FewerTransfers), ());
  settings::Set(kTransitPreferenceKey, value);
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

RoutingOptionsClassifier::RoutingOptionsClassifier(bool includeDirtyRoadClasses)
{
  Classificator const & c = classif();

  std::pair<std::vector<std::string>, RoutingOptions::Road> const types[] = {
      {{"highway", "motorway"}, RoutingOptions::Road::Motorway},

      {{"hwtag", "toll"}, RoutingOptions::Road::Toll},

      {{"route", "ferry"}, RoutingOptions::Road::Ferry},

      {{"psurface", "unpaved_bad"}, RoutingOptions::Road::Dirty},
      {{"psurface", "unpaved_good"}, RoutingOptions::Road::Dirty},

      {{"highway", "steps"}, RoutingOptions::Road::Steps}};

  char const * const dirtyRoadClasses[] = {"track", "road"};
  m_data.Reserve(std::size(types) + (includeDirtyRoadClasses ? std::size(dirtyRoadClasses) : 0));
  for (auto const & data : types)
    m_data.Insert(c.GetTypeByPath(data.first), data.second);
  if (includeDirtyRoadClasses)
    for (auto const * roadClass : dirtyRoadClasses)
      m_data.Insert(c.GetTypeByPath({"highway", roadClass}), RoutingOptions::Dirty);
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

RoutingOptionsClassifier const & RoutingOptionsClassifier::Instance(VehicleType vehicleType)
{
  // Road classes alone do not establish an unpaved surface for cycling.
  static RoutingOptionsClassifier const withRoadClasses(true);
  static RoutingOptionsClassifier const surfaceOnly(false);
  switch (vehicleType)
  {
  case VehicleType::Bicycle: return surfaceOnly;
  case VehicleType::Car:
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return withRoadClasses;
  case VehicleType::Count: break;
  }
  UNREACHABLE();
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
  if (vehicleType == VehicleType::Bicycle)
  {
    if (settings::Get(kBicycleSurfacePreferenceKey, value))
      m_savedBicycleSurface = value;
    settings::Delete(kBicycleSurfacePreferenceKey);
  }
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
  if (m_vehicleType == VehicleType::Bicycle)
  {
    if (m_savedBicycleSurface)
      settings::Set(kBicycleSurfacePreferenceKey, *m_savedBicycleSurface);
    else
      settings::Delete(kBicycleSurfacePreferenceKey);
  }
}

}  // namespace routing

#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <sstream>

namespace routing
{
// RoutingOptions -------------------------------------------------------------------------------------

std::string_view constexpr kAvoidRoutingOptionSettingsForCar = "avoid_routing_options_car";
std::string_view constexpr kRoutingOptionSettingsForBicycle = "routing_options_bicycle";

RoutingOptions LoadOptionsFromSettings(std::string_view key)
{
  uint32_t mode = 0;
  if (!settings::Get(key, mode))
    mode = 0;

  return RoutingOptions(base::checked_cast<RoutingOptions::RoadType>(mode));
}

void SaveOptionsToSettings(RoutingOptions options, std::string_view key)
{
  settings::Set(key, strings::to_string(static_cast<int32_t>(options.GetOptions())));
}

// static
RoutingOptions RoutingOptions::LoadCarOptionsFromSettings()
{
  return LoadOptionsFromSettings(kAvoidRoutingOptionSettingsForCar);
}

// static
void RoutingOptions::SaveCarOptionsToSettings(RoutingOptions options)
{
  SaveOptionsToSettings(options, kAvoidRoutingOptionSettingsForCar);
}

// static
RoutingOptions RoutingOptions::LoadBicycleOptionsFromSettings()
{
  return LoadOptionsFromSettings(kRoutingOptionSettingsForBicycle);
}

// static
void RoutingOptions::SaveBicycleOptionsToSettings(RoutingOptions options)
{
  SaveOptionsToSettings(options, kRoutingOptionSettingsForBicycle);
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
  append(RoutingOptions::Road::PublicBicycle);

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
  case RoutingOptions::Road::PublicBicycle: return "public_bicycle";
  case RoutingOptions::Road::Usual: return "usual";
  case RoutingOptions::Road::Max: return "max";
  }

  UNREACHABLE();
}

RoutingOptionSetter::RoutingOptionSetter(RoutingOptions::RoadType roadsMask)
{
  m_saved = RoutingOptions::LoadCarOptionsFromSettings();
  RoutingOptions::SaveCarOptionsToSettings(RoutingOptions(roadsMask));
}

RoutingOptionSetter::~RoutingOptionSetter()
{
  RoutingOptions::SaveCarOptionsToSettings(m_saved);
}

}  // namespace routing

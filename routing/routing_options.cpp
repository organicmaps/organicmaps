#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <initializer_list>
#include <sstream>
#include <utility>

using namespace std;

namespace routing
{

// RoutingOptions -------------------------------------------------------------------------------------

string const RoutingOptions::kAvoidRoutingOptionSettingsForCar = "avoid_routing_options_car";

// static
RoutingOptions RoutingOptions::LoadCarOptionsFromSettings()
{
  uint32_t mode = 0;
  if (!settings::Get(kAvoidRoutingOptionSettingsForCar, mode))
    mode = 0;

  return RoutingOptions(base::checked_cast<RoadType>(mode));
}

// static
void RoutingOptions::SaveCarOptionsToSettings(RoutingOptions options)
{
  settings::Set(kAvoidRoutingOptionSettingsForCar,
                strings::to_string(static_cast<int32_t>(options.GetOptions())));
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

  initializer_list<pair<vector<string>, RoutingOptions::Road>> const types = {
    {{"highway", "motorway"},             RoutingOptions::Road::Motorway},

    {{"hwtag", "toll"},                   RoutingOptions::Road::Toll},

    {{"route", "ferry"},                  RoutingOptions::Road::Ferry},
    {{"route", "ferry", "motorcar"},      RoutingOptions::Road::Ferry},
    {{"route", "ferry", "motor_vehicle"}, RoutingOptions::Road::Ferry},

    {{"highway", "track"},                RoutingOptions::Road::Dirty},
    {{"highway", "road"},                 RoutingOptions::Road::Dirty},
    {{"psurface", "unpaved_bad"},         RoutingOptions::Road::Dirty},
    {{"psurface", "unpaved_good"},        RoutingOptions::Road::Dirty}
  };

  for (auto const & data : types)
  {
    auto const & stringType = data.first;
    auto const & optionType = data.second;

    ASSERT_EQUAL(m_data.count(c.GetTypeByPath(stringType)), 0, ());
    m_data[c.GetTypeByPath(stringType)] = optionType;
  }
}

optional<RoutingOptions::Road> RoutingOptionsClassifier::Get(uint32_t type) const
{
  auto const it = m_data.find(type);
  if (it == m_data.cend())
    return {};

  return it->second;
}

RoutingOptionsClassifier const & RoutingOptionsClassifier::Instance()
{
  static RoutingOptionsClassifier instance;
  return instance;
}

RoutingOptions::Road ChooseMainRoutingOptionRoad(RoutingOptions options)
{
  if (options.Has(RoutingOptions::Road::Toll))
    return RoutingOptions::Road::Toll;

  if (options.Has(RoutingOptions::Road::Ferry))
    return RoutingOptions::Road::Ferry;

  if (options.Has(RoutingOptions::Road::Dirty))
    return RoutingOptions::Road::Dirty;

  if (options.Has(RoutingOptions::Road::Motorway))
    return RoutingOptions::Road::Motorway;

  return RoutingOptions::Road::Usual;
}

string DebugPrint(RoutingOptions const & routingOptions)
{
  ostringstream ss;
  ss << "RoutingOptions: {";

  bool wasAppended = false;
  auto const append = [&](RoutingOptions::Road road) {
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

  if (wasAppended)
    ss << " | ";

  ss << "}";

  return ss.str();
}

string DebugPrint(RoutingOptions::Road type)
{
  switch (type)
  {
    case RoutingOptions::Road::Toll: return "toll";
    case RoutingOptions::Road::Motorway: return "motorway";
    case RoutingOptions::Road::Ferry: return "ferry";
    case RoutingOptions::Road::Dirty: return "dirty";
    case RoutingOptions::Road::Usual: return "usual";
    case RoutingOptions::Road::Max: return "max";
  }

  UNREACHABLE();
}
}  // namespace routing

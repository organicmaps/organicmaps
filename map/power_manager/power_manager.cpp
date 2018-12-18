#include "map/power_manager/power_manager.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/serdes_json.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <unordered_map>

namespace
{
std::unordered_map<PowerManager::Config, PowerManager::FacilitiesState> const kConfigToState =
{
  {PowerManager::Config::Normal, {{true, true}}},
  {PowerManager::Config::Economy, {{false, false}}}
};

std::string GetFilePath()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "power_manager_config");
}
}  // namespace

void PowerManager::Load()
{
  Data result;
  try
  {
    FileReader reader(GetFilePath());
    NonOwningReaderSource source(reader);

    coding::DeserializerJson des(source);
    des(result);

    m_data = result;
    for (size_t i = 0; i < m_data.m_facilities.size(); ++i)
    {
      CHECK_NOT_EQUAL(static_cast<Facility>(i), Facility::Count, ());

      for (auto & subscriber : m_subscribers)
        subscriber->OnFacilityStateChanged(static_cast<Facility>(i), m_data.m_facilities[i]);
    }

    for (auto & subscriber : m_subscribers)
      subscriber->OnConfigChanged(m_data.m_config);
  }
  catch (base::Json::Exception & ex)
  {
    LOG(LERROR, ("Cannot deserialize power manager data from file. Exception:", ex.Msg()));
  }
  catch (FileReader::Exception const & ex)
  {
    LOG(LERROR, ("Cannot read power manager config file. Exception:", ex.Msg()));
  }

  // Reset to default state.
  m_data = {};
}

void PowerManager::SetFacility(Facility const facility, bool state)
{
  CHECK_NOT_EQUAL(facility, Facility::Count, ());

  if (m_data.m_facilities[static_cast<size_t>(facility)] == state)
    return;

  m_data.m_facilities[static_cast<size_t>(facility)] = state;

  bool isConfigChanged = BalanceConfig();

  if (!Save())
    return;

  for (auto & subscriber : m_subscribers)
    subscriber->OnFacilityStateChanged(facility, state);

  if (isConfigChanged)
  {
    for (auto & subscriber : m_subscribers)
      subscriber->OnConfigChanged(m_data.m_config);
  }
}

void PowerManager::SetConfig(Config const config)
{
  if (m_data.m_config == config)
    return;

  m_data.m_config = config;

  if (m_data.m_config == Config::None || m_data.m_config == Config::Auto)
    return;

  auto actualState = kConfigToState.at(config);

  if (m_data.m_facilities == actualState)
    return;

  std::swap(m_data.m_facilities, actualState);

  if (!Save())
    return;

  for (auto & subscriber : m_subscribers)
    subscriber->OnConfigChanged(m_data.m_config);

  for (size_t i = 0; i < actualState.size(); ++i)
  {
    if (m_data.m_facilities[i] != actualState[i])
    {
      for (auto & subscriber : m_subscribers)
        subscriber->OnFacilityStateChanged(static_cast<Facility>(i), m_data.m_facilities[i]);
    }
  }
}

bool PowerManager::GetFacility(Facility const facility) const
{
  CHECK_NOT_EQUAL(facility, Facility::Count, ());

  return m_data.m_facilities[static_cast<size_t>(facility)];
}

PowerManager::FacilitiesState const & PowerManager::GetFacilities() const
{
  return m_data.m_facilities;
}

PowerManager::Config const & PowerManager::GetConfig() const
{
  return m_data.m_config;
}

void PowerManager::OnBatteryLevelChanged(uint8_t level)
{
  if (m_data.m_config != Config::Auto)
    return;

  // TODO.
}

void PowerManager::Subscribe(Subscriber * subscriber)
{
  ASSERT(subscriber, ());
  m_subscribers.push_back(subscriber);
}

void PowerManager::UnsubscribeAll()
{
  m_subscribers.clear();
}

bool PowerManager::BalanceConfig()
{
  bool found = false;
  Config actualConfig = m_data.m_config;
  for (auto const & item : kConfigToState)
  {
    if (item.second == m_data.m_facilities)
    {
      actualConfig = item.first;
      found = true;
      break;
    }
  }

  if (!found)
    actualConfig = Config::None;

  if (m_data.m_config == actualConfig)
    return false;

  m_data.m_config = actualConfig;
  return true;
}

bool PowerManager::Save()
{
  auto const result = base::WriteToTempAndRenameToFile(GetFilePath(), [this](string const & fileName)
  {
    try
    {
      FileWriter writer(fileName);
      coding::SerializerJson<FileWriter> ser(writer);
      ser(m_data);
      return true;
    }
    catch (base::Json::Exception & ex)
    {
      LOG(LERROR, ("Cannot serialize power manager data into file. Exception:", ex.Msg()));
    }
    catch (FileReader::Exception const & ex)
    {
      LOG(LERROR, ("Cannot write power manager file. Exception:", ex.Msg()));
    }

    return false;
  });

  if (result)
    return true;

  // Try to load last correct state and notify subscribers.
  Load();
  return false;
}

std::string DebugPrint(PowerManager::Facility const facility)
{
  switch (facility)
  {
  case PowerManager::Facility::ThreeDimensionalBuildings: return "ThreeDimensionalBuildings";
  case PowerManager::Facility::TrackRecord: return "TrackRecord";
  case PowerManager::Facility::Count: return "Count";
  }
}

std::string DebugPrint(PowerManager::Config const config)
{
  switch (config)
  {
  case PowerManager::Config::None: return "None";
  case PowerManager::Config::Normal: return "Normal";
  case PowerManager::Config::Economy: return "Economy";
  case PowerManager::Config::Auto: return "Auto";
  }
}

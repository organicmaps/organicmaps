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
using Subscribers = std::vector<PowerManager::Subscriber *>;

std::unordered_map<PowerManager::Scheme, PowerManager::FacilitiesState> const kSchemeToState =
{
  {PowerManager::Scheme::Normal, {{true, true}}},
  {PowerManager::Scheme::Economy, {{false, false}}}
};

std::string GetFilePath()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "power_manager_config");
}

void NotifySubscribers(Subscribers & subscribers, PowerManager::Scheme const scheme)
{
  for (auto & subscriber : subscribers)
    subscriber->OnPowerSchemeChanged(scheme);
}

void NotifySubscribers(Subscribers & subscribers, PowerManager::Facility const facility,
                       bool enabled)
{
  for (auto & subscriber : subscribers)
    subscriber->OnPowerFacilityChanged(facility, enabled);
}
}  // namespace

void PowerManager::Load()
{
  try
  {
    FileReader reader(GetFilePath());
    NonOwningReaderSource source(reader);

    coding::DeserializerJson des(source);
    Config result;
    des(result);

    m_config = result;
    for (size_t i = 0; i < m_config.m_facilities.size(); ++i)
    {
      NotifySubscribers(m_subscribers, static_cast<Facility>(i), m_config.m_facilities[i]);
    }

    NotifySubscribers(m_subscribers, m_config.m_scheme);
    return;
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
  m_config = {};
}

void PowerManager::SetFacility(Facility const facility, bool enabled)
{
  CHECK_NOT_EQUAL(facility, Facility::Count, ());

  if (m_config.m_facilities[static_cast<size_t>(facility)] == enabled)
    return;

  m_config.m_facilities[static_cast<size_t>(facility)] = enabled;

  auto const isSchemeChanged = BalanceScheme();

  if (!Save())
    return;

  NotifySubscribers(m_subscribers, facility, enabled);

  if (isSchemeChanged)
    NotifySubscribers(m_subscribers, m_config.m_scheme);
}

void PowerManager::SetScheme(Scheme const scheme)
{
  if (m_config.m_scheme == scheme)
    return;

  m_config.m_scheme = scheme;

  if (m_config.m_scheme == Scheme::None || m_config.m_scheme == Scheme::Auto)
    return;

  auto actualState = kSchemeToState.at(scheme);

  if (m_config.m_facilities == actualState)
    return;

  std::swap(m_config.m_facilities, actualState);

  if (!Save())
    return;

  NotifySubscribers(m_subscribers, m_config.m_scheme);

  for (size_t i = 0; i < actualState.size(); ++i)
  {
    if (m_config.m_facilities[i] != actualState[i])
      NotifySubscribers(m_subscribers, static_cast<Facility>(i), m_config.m_facilities[i]);
  }
}

bool PowerManager::IsFacilityEnabled(Facility const facility) const
{
  CHECK_NOT_EQUAL(facility, Facility::Count, ());

  return m_config.m_facilities[static_cast<size_t>(facility)];
}

PowerManager::FacilitiesState const & PowerManager::GetFacilities() const
{
  return m_config.m_facilities;
}

PowerManager::Scheme const & PowerManager::GetScheme() const
{
  return m_config.m_scheme;
}

void PowerManager::OnBatteryLevelChanged(uint8_t level)
{
  CHECK_LESS_OR_EQUAL(level, 100, ());

  if (m_config.m_scheme != Scheme::Auto)
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

bool PowerManager::BalanceScheme()
{
  bool found = false;
  Scheme actualScheme = m_config.m_scheme;
  for (auto const & item : kSchemeToState)
  {
    if (item.second == m_config.m_facilities)
    {
      actualScheme = item.first;
      found = true;
      break;
    }
  }

  if (!found)
    actualScheme = Scheme::None;

  if (m_config.m_scheme == actualScheme)
    return false;

  m_config.m_scheme = actualScheme;
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
      ser(m_config);
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
  case PowerManager::Facility::Buildings3d: return "Buildings3d";
  case PowerManager::Facility::TrackRecord: return "TrackRecord";
  case PowerManager::Facility::Count: return "Count";
  }
}

std::string DebugPrint(PowerManager::Scheme const scheme)
{
  switch (scheme)
  {
  case PowerManager::Scheme::None: return "None";
  case PowerManager::Scheme::Normal: return "Normal";
  case PowerManager::Scheme::Economy: return "Economy";
  case PowerManager::Scheme::Auto: return "Auto";
  }
}

#pragma once

#include "base/visitor.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Note: this class is NOT thread-safe.
class PowerManager
{
public:
  // Note: the order is important.
  // Note: new facilities must be added before Facility::Count.
  // Note: do not use Facility::Count in external code, this value for internal use only.
  enum class Facility : uint8_t
  {
    ThreeDimensionalBuildings,
    TrackRecord,

    Count
  };

  using FacilitiesState = std::array<bool, static_cast<size_t>(Facility::Count)>;

  enum class Config : uint8_t
  {
    None,
    Normal,
    Economy,
    Auto
  };

  class Subscriber
  {
  public:
    virtual ~Subscriber() = default;

    virtual void OnFacilityStateChanged(Facility const facility, bool state) {}

    virtual void OnConfigChanged(Config const actualConfig) {}
  };

  void Load();
  void SetFacility(Facility const facility, bool state);
  void SetConfig(Config const config);
  bool GetFacility(Facility const facility) const;
  FacilitiesState const & GetFacilities() const;
  Config const & GetConfig() const;

  void OnBatteryLevelChanged(uint8_t level);

  void Subscribe(Subscriber * subscriber);
  void UnsubscribeAll();

private:
  struct Data
  {
    DECLARE_VISITOR(visitor(m_facilities, "current_state"), visitor(m_config, "config"));

    FacilitiesState m_facilities = {true, true};
    Config m_config = Config::Normal;
  };

  // Returns true when config was changed.
  bool BalanceConfig();
  bool Save();

  std::vector<Subscriber *> m_subscribers;

  Data m_data;
};

std::string DebugPrint(PowerManager::Facility const facility);
std::string DebugPrint(PowerManager::Config const config);

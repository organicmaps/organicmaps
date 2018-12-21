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
    Buildings3d,
    TrackRecord,

    Count
  };

  using FacilitiesState = std::array<bool, static_cast<size_t>(Facility::Count)>;

  enum class Scheme : uint8_t
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

    virtual void OnPowerFacilityChanged(Facility const facility, bool enabled) {}
    virtual void OnPowerSchemeChanged(Scheme const actualScheme) {}
  };

  void Load();
  void SetFacility(Facility const facility, bool enabled);
  void SetScheme(Scheme const scheme);
  bool IsFacilityEnabled(Facility const facility) const;
  FacilitiesState const & GetFacilities() const;
  Scheme const & GetScheme() const;

  void OnBatteryLevelChanged(uint8_t level);

  void Subscribe(Subscriber * subscriber);
  void UnsubscribeAll();

private:
  struct Config
  {
    DECLARE_VISITOR(visitor(m_facilities, "current_state"), visitor(m_scheme, "scheme"));

    FacilitiesState m_facilities = {{true, true}};
    Scheme m_scheme = Scheme::Normal;
  };

  // Returns true when scheme was changed.
  bool BalanceScheme();
  bool Save();

  std::vector<Subscriber *> m_subscribers;

  Config m_config;
};

std::string DebugPrint(PowerManager::Facility const facility);
std::string DebugPrint(PowerManager::Scheme const scheme);

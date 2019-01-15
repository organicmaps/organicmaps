#pragma once

#include "map/power_management/power_management_schemas.hpp"

#include "base/visitor.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace power_management
{
// Note: this class is NOT thread-safe.
class PowerManager
{
public:
  class Subscriber
  {
  public:
    virtual ~Subscriber() = default;

    virtual void OnPowerFacilityChanged(Facility const facility, bool enabled) {}
    virtual void OnPowerSchemeChanged(Scheme const actualScheme) {}
  };

  void Load();
  // Set some facility state manually, it turns current scheme to Scheme::None.
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

    Config() { m_facilities.fill(true); }

    FacilitiesState m_facilities;
    Scheme m_scheme = Scheme::Normal;
  };

  bool Save();

  std::vector<Subscriber *> m_subscribers;

  Config m_config;
};
}  // namespace power_management

#include "routing/speed_camera_prohibition.hpp"

#include "base/string_utils.hpp"

#include <vector>

namespace
{
// List of country names where mwm should be generated without speed cameras.
std::vector<std::string> kSpeedCamerasProhibitedCountries = {
    "Cyprus", "Macedonia", "Switzerland", "Turkey",
};

// List of country names where an end user should be warned about speed cameras.
std::vector<std::string> kSpeedCamerasPartlyProhibitedCountries = {
    "France", "Germany",
};

bool IsMwmContained(platform::CountryFile const & mwm, std::vector<std::string> const & countryList)
{
  for (auto const & country : countryList)
  {
    if (strings::StartsWith(mwm.GetName(), country))
      return true;
  }

  return false;
}
}  // namespace

namespace routing
{
bool AreSpeedCamerasProhibited(platform::CountryFile const & mwm)
{
  return IsMwmContained(mwm, kSpeedCamerasProhibitedCountries);
}

bool AreSpeedCamerasPartlyProhibited(platform::CountryFile const & mwm)
{
  return IsMwmContained(mwm, kSpeedCamerasPartlyProhibitedCountries);
}
} // namespace routing

#include "routing/speed_camera_prohibition.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <vector>

namespace
{
// List of country names where mwm should be generated without speed cameras.
std::vector<std::string> kSpeedCamerasProhibitedCountries = {
    "Macedonia",
    "Switzerland",
    "Turkey",
};

// List of country names where an end user should be warned about speed cameras.
std::vector<std::string> kSpeedCamerasPartlyProhibitedCountries = {
    "France",
    "Germany",
};

bool IsMwmContained(platform::CountryFile const & mwm, std::vector<std::string> const & countryList)
{
  return std::any_of(countryList.cbegin(), countryList.cend(),
                     [&mwm](auto const & country) { return mwm.GetName().starts_with(country); });
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
}  // namespace routing

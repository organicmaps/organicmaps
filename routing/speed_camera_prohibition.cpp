#include "routing/speed_camera_prohibition.hpp"

#include "base/string_utils.hpp"

#include <vector>

namespace
{
// List of country names where mwm should be generated without speedcameras.
std::vector<std::string> kCountryBlockListForMapGeneration = {
    "Cyprus",
    "Macedonia",
    "Switzerland",
    "Turkey",
};

// List of country names where an end user should be warned about speedcameras.
std::vector<std::string> kCountryWarnList = {
    "France",
    "Germany",
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
bool ShouldRemoveSpeedcamWhileMapGeneration(platform::CountryFile const & mwm)
{
  return IsMwmContained(mwm, kCountryBlockListForMapGeneration);
}

bool ShouldWarnAboutSpeedcam(platform::CountryFile const & mwm)
{
  return ShouldRemoveSpeedcamWhileMapGeneration(mwm) || IsMwmContained(mwm, kCountryWarnList);
}
} // namespace routing

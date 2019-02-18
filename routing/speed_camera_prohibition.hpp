#pragma once

#include "platform/country_file.hpp"

namespace routing
{
/// \returns true if any information about speed cameras is prohibited in |mwm|.
bool AreSpeedCamerasProhibited(platform::CountryFile const & mwm);

/// \returns true if any information about speed cameras is prohibited or partly prohibited in |mwm|.
bool AreSpeedCamerasPartlyProhibited(platform::CountryFile const & mwm);
} // namespace routing

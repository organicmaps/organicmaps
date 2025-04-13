#pragma once

#include <string>

#include "platform/measurement_utils.hpp"

namespace platform
{
struct LocalizedUnits
{
  std::string m_low;
  std::string m_high;
};

extern std::string GetLocalizedTypeName(std::string const & type);
extern std::string GetLocalizedBrandName(std::string const & brand);
extern std::string GetLocalizedString(std::string const & key);
extern std::string GetCurrencySymbol(std::string const & currencyCode);
extern std::string GetLocalizedMyPositionBookmarkName();

extern LocalizedUnits const & GetLocalizedDistanceUnits();
extern LocalizedUnits const & GetLocalizedAltitudeUnits();

extern std::string const & GetLocalizedSpeedUnits(measurement_utils::Units units);
extern std::string const & GetLocalizedSpeedUnits();
}  // namespace platform

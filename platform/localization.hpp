#pragma once

#include <string>

namespace measurement_utils
{
enum class Units;
}

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

extern LocalizedUnits GetLocalizedDistanceUnits();
extern LocalizedUnits GetLocalizedAltitudeUnits();

extern const std::string & GetLocalizedSpeedUnits(measurement_utils::Units units);
extern std::string GetLocalizedSpeedUnits();
}  // namespace platform

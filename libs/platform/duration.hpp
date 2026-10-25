#pragma once

#include "platform/localization.hpp"

#include <chrono>
#include <set>
#include <string>

namespace platform
{

class Duration
{
public:
  enum class Units
  {
    Days = 0,
    Hours = 1,
    Minutes = 2,
    Seconds = 3,
  };

  /// Truncates fractional seconds before applying display formatting.
  explicit Duration(double seconds);

  static std::string GetUnitsString(Units unit);

  std::string GetLocalizedString(std::initializer_list<Units> units, Locale const & locale) const;
  std::string GetPlatformLocalizedString() const;

  /// Duration label rounded to the nearest minute (half up), with a one-minute minimum.
  /// Omits zero minutes and retains hours above 24; arrival calculations must use the raw duration.
  std::string GetRoutingString() const;

private:
  explicit Duration(std::chrono::seconds seconds) : m_seconds(seconds) {}

  std::chrono::seconds const m_seconds;

  std::string GetString(std::initializer_list<Units> units, std::string_view unitSeparator,
                        std::string_view groupingSeparator) const;
};

std::string DebugPrint(Duration::Units units);

}  // namespace platform

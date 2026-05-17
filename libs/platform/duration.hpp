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

  explicit Duration(unsigned long seconds);

  static std::string GetUnitsString(Units unit);

  std::string GetLocalizedString(std::initializer_list<Units> units, Locale const & locale) const;
  std::string GetPlatformLocalizedString() const;

  /// \brief Hours + minutes only, seconds dropped. Mirrors Android's Utils.formatRoutingTime style:
  /// "<m> min" for sub-hour durations, "<h> h <m> min" otherwise. Used for compact ETA labels
  /// (e.g. balloons on route variants); reuse from C++ when you'd otherwise round-trip through JNI.
  std::string GetHoursMinutesString() const;

private:
  std::chrono::seconds const m_seconds;

  std::string GetString(std::initializer_list<Units> units, std::string_view unitSeparator,
                        std::string_view groupingSeparator) const;
};

std::string DebugPrint(Duration::Units units);

}  // namespace platform

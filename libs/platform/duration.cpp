#include "duration.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

/// @todo(KK): move the formatting code from the platform namespace
namespace platform
{
namespace
{
using namespace std::chrono;

static constexpr std::string_view kNoSpace = "";

unsigned long SecondsToUnits(seconds duration, Duration::Units unit)
{
  switch (unit)
  {
  case Duration::Units::Days: return duration_cast<days>(duration).count();
  case Duration::Units::Hours: return duration_cast<hours>(duration).count();
  case Duration::Units::Minutes: return duration_cast<minutes>(duration).count();
  case Duration::Units::Seconds: return duration_cast<seconds>(duration).count();
  default: UNREACHABLE();
  }
}

seconds UnitsToSeconds(long value, Duration::Units unit)
{
  switch (unit)
  {
  case Duration::Units::Days: return days(value);
  case Duration::Units::Hours: return hours(value);
  case Duration::Units::Minutes: return minutes(value);
  case Duration::Units::Seconds: return seconds(value);
  default: UNREACHABLE();
  }
}

std::string_view GetUnitSeparator(Locale const & locale)
{
  static constexpr auto kEmptyNumberUnitSeparatorLocales =
      std::array{"en", "de", "fr", "he", "fa", "ja", "ko", "mr", "th", "tr", "vi", "zh"};
  bool const isEmptySeparator = base::IsExist(kEmptyNumberUnitSeparatorLocales, locale.m_language);
  return isEmptySeparator ? kNoSpace : kNarrowNonBreakingSpace;
}

std::string_view GetUnitsGroupingSeparator(Locale const & locale)
{
  static constexpr auto kEmptyGroupingSeparatorLocales = std::array{"ja", "zh"};
  bool const isEmptySeparator = base::IsExist(kEmptyGroupingSeparatorLocales, locale.m_language);
  return isEmptySeparator ? kNoSpace : kNonBreakingSpace;
}

struct PlatformSeparators
{
  std::string_view m_unitSep, m_groupingSep;

  PlatformSeparators()
  {
    auto const locale = GetCurrentLocale();
    m_unitSep = GetUnitSeparator(locale);
    m_groupingSep = GetUnitsGroupingSeparator(locale);
  }
};

PlatformSeparators const & GetPlatformSeparators()
{
  static PlatformSeparators const separators;
  return separators;
}
}  // namespace

Duration::Duration(double seconds)
  : m_seconds(duration_cast<std::chrono::seconds>(std::chrono::duration<double>(seconds)))
{
  ASSERT_GREATER_OR_EQUAL(seconds, 0.0, ());
}

std::string Duration::GetLocalizedString(std::initializer_list<Units> units, Locale const & locale) const
{
  return GetString(units, GetUnitSeparator(locale), GetUnitsGroupingSeparator(locale));
}

std::string Duration::GetPlatformLocalizedString() const
{
  auto const & seps = GetPlatformSeparators();
  return GetString({Units::Days, Units::Hours, Units::Minutes, Units::Seconds}, seps.m_unitSep, seps.m_groupingSep);
}

std::string Duration::GetString(std::initializer_list<Units> units, std::string_view unitSeparator,
                                std::string_view groupingSeparator) const
{
  ASSERT(units.size(), ());
  ASSERT(base::IsSortedAndUnique(units), ());

  if (SecondsToUnits(m_seconds, Units::Minutes) == 0)
  {
    unsigned long const secs = SecondsToUnits(m_seconds, Units::Seconds);
    return std::to_string(secs).append(unitSeparator).append(GetUnitsString(Units::Seconds));
  }

  std::string formattedTime;
  seconds remainingSeconds = seconds(m_seconds);

  bool const skipSeconds = SecondsToUnits(m_seconds, Units::Minutes) >= 10;
  for (auto const unit : units)
  {
    if (unit == Units::Seconds && skipSeconds)
      continue;
    unsigned long const unitsCount = SecondsToUnits(remainingSeconds, unit);
    if (unitsCount > 0)
    {
      if (!formattedTime.empty())
        formattedTime.append(groupingSeparator);
      formattedTime.append(std::to_string(unitsCount).append(unitSeparator).append(GetUnitsString(unit)));
      remainingSeconds -= UnitsToSeconds(unitsCount, unit);
    }
  }
  return formattedTime;
}

std::string Duration::GetRoutingString() const
{
  seconds const rounded = std::max(minutes(1), duration_cast<minutes>(m_seconds + seconds(30)));
  auto const & seps = GetPlatformSeparators();
  return Duration(rounded).GetString({Units::Hours, Units::Minutes}, seps.m_unitSep, seps.m_groupingSep);
}

std::string Duration::GetUnitsString(Units unit)
{
  switch (unit)
  {
  case Units::Minutes: return platform::GetLocalizedString("minute");
  case Units::Hours: return platform::GetLocalizedString("hour");
  case Units::Days: return platform::GetLocalizedString("day");
  case Units::Seconds: return platform::GetLocalizedString("second");
  default: UNREACHABLE();
  }
}

std::string DebugPrint(Duration::Units units)
{
  switch (units)
  {
  case Duration::Units::Days: return "d";
  case Duration::Units::Hours: return "h";
  case Duration::Units::Minutes: return "m";
  case Duration::Units::Seconds: return "s";
  default: UNREACHABLE();
  }
}
}  // namespace platform

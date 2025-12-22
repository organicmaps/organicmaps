#include "local_timezone.hpp"

#include <ctime>

namespace om::tz
{
TimeZone GetLocalTimeZone()
{
  // Get current time in the local timezone
  time_t const now = std::time(nullptr);

  // Get broken-down time
  std::tm local{};
  std::tm utc{};
  localtime_r(&now, &local);
  gmtime_r(&now, &utc);

  bool const isDst = local.tm_isdst > 0;

  // Convert back to time_t
  time_t const localAsUtc = timegm(&local);
  time_t const utcAsUtc = timegm(&utc);

  // Compute offset
  // This gives the current total offset including DST
  auto const total_offset_sec = static_cast<int32_t>(difftime(localAsUtc, utcAsUtc));

  // Determine base offset and DST delta
  // To separate base offset and DST delta, probe a nearby non-DST date - Jan 1st.
  std::tm jan{};
  jan.tm_year = local.tm_year;
  jan.tm_mon = 0;
  jan.tm_mday = 1;
  jan.tm_hour = 12;

  time_t const janLocal = mktime(&jan);
  std::tm janUtc{};
  gmtime_r(&janLocal, &janUtc);

  auto const janOffset = static_cast<int32_t>(difftime(timegm(&jan), timegm(&janUtc)));

  TimeZone localTz{
      .generation_year_offset = static_cast<uint16_t>(local.tm_year + 1900 - TimeZone::kGenerationYearStart),
      .base_offset = static_cast<uint8_t>(janOffset / 60 / 15 + 64),
      .dst_delta = static_cast<int16_t>((total_offset_sec - janOffset) / 60),
      .transitions = {}};
  if (isDst)
  {
    // We cannot determine DST rules for the local timezone at runtime.
    // Assume it is always DST.
    localTz.transitions = {
        {0, 0, 1},
        {365 * 5, 0, 0},
    };
  }

  return localTz;
}
}  // namespace om::tz

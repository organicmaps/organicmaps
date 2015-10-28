#include "rules_evaluation.hpp"
// #include "rules_evaluation_private.hpp"
#include <tuple>

namespace
{
using THourMinutes = std::tuple<int, int>;

bool ToHourMinutes(osmoh::Time const & t, THourMinutes & hm)
{
  if (!t.IsHoursMinutes())
    return false;
  hm = THourMinutes{t.GetHoursCount(), t.GetMinutesCount()};
  return true;
}

bool ToHourMinutes(std::tm const & t, THourMinutes & hm)
{
  hm = THourMinutes{t.tm_hour, t.tm_min};
  return true;
}
} // namespace

namespace osmoh
{
bool IsActive(Timespan const & span, std::tm const & time)
{
  if (span.HasStart() && span.HasEnd())
  {
    THourMinutes start;
    THourMinutes end;
    THourMinutes toBeChecked;
    if (!ToHourMinutes(span.GetStart(), start))
      return false;
    if (!ToHourMinutes(span.GetEnd(), end))
      return false;
    if (!ToHourMinutes(time, toBeChecked))
      return false;

    return start <= toBeChecked && toBeChecked <= end;

  }
  return false;
}
} // namespace osmoh

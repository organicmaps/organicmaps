#pragma once

#include "osm_time_range.hpp"
#include <ctime>

namespace osmoh
{
bool IsActive(Timespan const & spsn, std::tm const & date);
bool IsActive(WeekdayRange const & range, std::tm const & date);
bool IsActive(Holiday const & holiday, std::tm const & date);
bool IsActive(Weekdays const & weekdays, std::tm const & date);
bool IsActive(MonthdayRange const & range, std::tm const & date);
bool IsActive(YearRange const & range, std::tm const & date);
bool IsActive(WeekRange const & range, std::tm const & date);
bool IsActive(RuleSequence const & rule, std::tm const & date);
} // namespace osmoh


// bool IsActive(Time const & time, std::tm const & date);
// bool IsActive(NthEntry const & nthEntry, std::tm const & date);
// bool IsActive(MonthDay const & MonthDay, std::tm const & date);

#pragma once

#include "opening_hours.hpp"

#include <ctime>

namespace osmoh
{
bool IsActive(Timespan span, std::tm const & date);
bool IsActive(WeekdayRange const & range, std::tm const & date);
bool IsActive(Holiday const & holiday, std::tm const & date, std::string const & countryId, std::string & holidayName);
bool IsActive(Weekdays const & weekdays, std::tm const & date, std::string const & countryId);
bool IsActive(MonthdayRange const & range, std::tm const & date);
bool IsActive(YearRange const & range, std::tm const & date);
bool IsActive(WeekRange const & range, std::tm const & date);
bool IsActive(RuleSequence const & rule, time_t const timestamp, std::string const & countryId);
} // namespace osmoh

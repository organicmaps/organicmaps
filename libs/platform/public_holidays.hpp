#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "3party/opening_hours/opening_hours.hpp"

namespace ph
{

//convert time_t to ISO date string (YYYY-MM-DD format)
std::string TimeTToISODate(time_t const & date);

//check if the country has holidays
bool HasHolidays(std::string const & countryId);

//load the holidays for a country (return empty map if not found)
std::unordered_map<std::string, std::string> LoadCountryHolidays(std::string const & countryId);

// load the dates of the holidays as time_t
osmoh::THolidayDates LoadHolidaysDate(std::string const & countryId);

// check if the date is a public holiday if not found should return false (nothing in the ui)
bool GetHolidayName(std::string const & countryId, time_t const dateTime, std::string & holidayName);

}//namespace ph


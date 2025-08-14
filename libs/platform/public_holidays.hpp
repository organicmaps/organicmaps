#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

#include "platform/platform.hpp"

namespace ph
{

//convert time_t to ISO date string (YYYY-MM-DD format)
std::string TimeTToISODate(time_t const dateTime);

//check if the country has holidays
bool HasHolidays(std::string const & countryName);

//load the holidays for a country (return empty map if not found)
std::unordered_map<std::string, std::string> LoadCountryHolidays(std::string const & countryName);

//check if the date is a public holiday if not found should return false (nothing in the ui)
bool GetHolidayName(std::string const & countryName,  time_t const dateTime, std::string & holidayName);

}//namespace ph


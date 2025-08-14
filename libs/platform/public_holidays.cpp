#include "platform/public_holidays.hpp"
#include "base/file_name_utils.hpp"  
#include "glaze/json.hpp"
#include "base/logging.hpp"
#include <ctime> 
#include <sstream>
#include <iomanip>

namespace ph
{

//convert time_t to ISO date string (YYYY-MM-DD format)
std::string TimeTToISODate(time_t const dateTime)
{
    std::tm tm = *std::localtime(&dateTime);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}


//check if the country has holidays
bool HasHolidays(std::string const & countryName)
{
    std::string const holidayFilePath = base::JoinPath("data", "countries", "public_holidays", countryName + ".json");
    return Platform::FileExists(holidayFilePath);

}


//load the holidays for a country
std::unordered_map<std::string, std::string> LoadCountryHolidays(std::string const & countryName)
{
   std::unordered_map<std::string, std::string> holidays;

   std::string const holidayFilePath = base::JoinPath("data", "countries", "public_holidays", countryName + ".json");
   
   try 
   {
    std::string jsonContent;
    GetPlatform().GetReader(holidayFilePath)->ReadAsString(jsonContent);  
    auto ec =glz::read_json(holidays,jsonContent);
    if (ec)
    {
        LOG(LWARNING,("Failed to parse holiday json for", countryName,":",glz::format_error(ec,jsonContent) ))
        return {};
    }
    return holidays;
    }

    catch (RootException const & ex) 
    {
        LOG(LWARNING, ("Can't load holidays for", countryName, ":", holidayFilePath, ex.what()));
        return {};
    }  
}

//check if the date is a public holiday if not found should return false (no thing in the ui)
bool GetHolidayName(std::string const & countryName, time_t const dateTime, std::string & holidayName)
{
    std::string isoDateString = TimeTToISODate(dateTime);
   
    auto holidays = LoadCountryHolidays(countryName);

    if(holidays.empty())
    {
        return false;
    }
    
        auto it = holidays.find(isoDateString);
        if (it != holidays.end())
        {
            holidayName = it->second;
            return true;
        }
        
        return false;
}



}//namespace ph

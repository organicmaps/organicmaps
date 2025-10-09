#include "platform/public_holidays.hpp"
#include "base/file_name_utils.hpp"  
#include "glaze/json.hpp"
#include "base/logging.hpp"
#include <ctime> 
#include <sstream>
#include <iomanip>

namespace ph
{

//convert std::tm to ISO date string (YYYY-MM-DD format)
std::string TimeTToISODate(time_t const & date)
{
  std::tm tm = *std::localtime(&date);
  std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

//check if the country has holidays
bool HasHolidays(std::string const & countryId)
{
    std::string const holidayFilePath = base::JoinPath("countries", "public_holidays", countryId + ".json");
    
    try
    {
        std::string const fullPath = GetPlatform().ReadPathForFile(holidayFilePath);
        return Platform::IsFileExistsByFullPath(fullPath);
    }
    catch (FileAbsentException const &)
    {
        return false;
    }
}

//load the holidays for a country
std::unordered_map<std::string, std::string> LoadCountryHolidays(std::string const & countryId)
{
   std::unordered_map<std::string, std::string> holidays;

   std::string const holidayFilePath = base::JoinPath("countries", "public_holidays", countryId + ".json");
   
   try 
   {
    std::string jsonContent;
    GetPlatform().GetReader(holidayFilePath)->ReadAsString(jsonContent);  
    auto ec =glz::read_json(holidays,jsonContent);
    if (ec)
    {
        LOG(LWARNING,("Failed to parse holiday json for", countryId,":",glz::format_error(ec,jsonContent) ));
        return {};
    }
    return holidays;
    }

    catch (RootException const & ex) 
    {
        LOG(LWARNING, ("Can't load holidays for", countryId, ":", holidayFilePath, ex.what()));
        return {};
    }  
}

//check if the date is a public holiday if not found should return false 
bool GetHolidayName(std::string const & countryId, time_t const dateTime, std::string & holidayName)
{
    std::string isoDateString = TimeTToISODate(dateTime);
   
    auto holidays = LoadCountryHolidays(countryId);

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

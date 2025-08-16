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
    std::string const holidayFilePath = base::JoinPath("countries", "public_holidays", countryName + ".json");
    
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
std::unordered_map<std::string, std::string> LoadCountryHolidays(std::string const & countryName)
{
   std::unordered_map<std::string, std::string> holidays;

   std::string const holidayFilePath = base::JoinPath("countries", "public_holidays", countryName + ".json");
   
   try 
   {
    std::string jsonContent;
    GetPlatform().GetReader(holidayFilePath)->ReadAsString(jsonContent);  
    auto ec =glz::read_json(holidays,jsonContent);
    if (ec)
    {
        LOG(LWARNING,("Failed to parse holiday json for", countryName,":",glz::format_error(ec,jsonContent) ));
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

//check if the date is a public holiday if not found should return false 
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

// Debug function to help verify path construction
std::string GetHolidayFilePath(std::string const & countryName)
{
    return base::JoinPath("countries", "public_holidays", countryName + ".json");
}

// Debug function to check if a holiday file exists and log the full path
bool DebugHolidayFileExists(std::string const & countryName)
{
    std::string const holidayFilePath = GetHolidayFilePath(countryName);
    
    try
    {
        std::string const fullPath = GetPlatform().ReadPathForFile(holidayFilePath);
        bool exists = Platform::IsFileExistsByFullPath(fullPath);
        
        if (exists)
        {
            LOG(LINFO, ("Holiday file found for", countryName, "at:", fullPath));
        }
        else
        {
            LOG(LWARNING, ("Holiday file not found for", countryName, "at:", fullPath));
        }
        
        return exists;
    }
    catch (FileAbsentException const & ex)
    {
        LOG(LWARNING, ("FileAbsentException for", countryName, ":", ex.what()));
        return false;
    }
    catch (std::exception const & ex)
    {
        LOG(LWARNING, ("Exception checking holiday file for", countryName, ":", ex.what()));
        return false;
    }
}

}//namespace ph

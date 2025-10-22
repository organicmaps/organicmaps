#include "platform/public_holidays.hpp"

#include "base/file_name_utils.hpp"

#include "glaze/json.hpp"

#include "base/logging.hpp"

#include "3party/opening_hours/opening_hours.hpp"

#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace ph
{
static std::unordered_map<std::string, std::unordered_map<std::string,std::string>> holidayCache;
static std::mutex holidayCacheMutex;

//convert std::tm to ISO date string (YYYY-MM-DD format)
std::string TimeTToISODate(time_t const & date)
{
  std::tm tm = *std::localtime(&date);
  std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

time_t ISODateToTimeT(std::string const & isoDate)
{
  std::tm tm = {};
  std::istringstream iss(isoDate);
  iss >> std::get_time(&tm, "%Y-%m-%d");
  if (iss.fail())
    throw std::runtime_error("Invalid date format, expected YYYY-MM-DD");
  return std::mktime(&tm);
}

// check if the country has holidays
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
  {
    std::lock_guard lock(holidayCacheMutex);
    auto it = holidayCache.find(countryId);
    if (it != holidayCache.end())
    {
        return it->second;
    }
  }

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
    {
      std::lock_guard lock(holidayCacheMutex);
      holidayCache[countryId] = holidays;
    }

     return holidays;
    }

    catch (RootException const & ex) 
    {
        LOG(LWARNING, ("Can't load holidays for", countryId, ":", holidayFilePath, ex.what()));
        return {};
    }  
}

// load the dates of the holidays as time_t
osmoh::THolidayDates LoadHolidaysDate(std::string const & countryId)
{
  auto holidays = LoadCountryHolidays(countryId);

  if (holidays.empty())
    return {};

  osmoh::THolidayDates holidayDates;
  holidayDates.reserve(holidays.size());

  for (auto const & [isoDate, name] : holidays)
  {
    try
    {
      time_t const date = ISODateToTimeT(isoDate);
      holidayDates.insert(date);
    }
    catch (std::runtime_error const & ex)
    {
      LOG(LWARNING, ("Invalid date format in holidays for", countryId, ":", isoDate, ex.what()));
    }
  }

  return holidayDates;
}

// check if the date is a public holiday if not found should return false
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

#include "region_meta.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"

#include "cppjansson/cppjansson.hpp"

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <vector>

namespace
{
int8_t ParseHolidayReference(std::string const & ref)
{
  if (ref == "easter")
    return feature::RegionData::PHReference::PH_EASTER;
  if (ref == "orthodox easter")
    return feature::RegionData::PHReference::PH_ORTHODOX_EASTER;
  if (ref == "victoriaDay")
    return feature::RegionData::PHReference::PH_VICTORIA_DAY;
  if (ref == "canadaDay")
    return feature::RegionData::PHReference::PH_CANADA_DAY;
  return 0;
}

time_t ParsePublicHolidayDate(std::string const & date)
{
  int y = 0, m = 0, d = 0;
  if (std::sscanf(date.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
    MYTHROW(base::Json::Exception, ("Bad public holiday date", date));

  std::tm tm{};
  tm.tm_year = y - 1900;
  tm.tm_mon = m - 1;
  tm.tm_mday = d;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;

  return timegm(&tm);
}
}  // namespace

namespace feature
{
bool ReadRegionDataImpl(std::string const & countryName, RegionData & data)
{
  /// @todo How LEAP_SPEEDS_FILE was generated before? It's always absent now.
  if (Platform::IsFileExistsByFullPath(LEAP_SPEEDS_FILE))
  {
    try
    {
      auto crossMwmDataReader = GetPlatform().GetReader(LEAP_SPEEDS_FILE);
      std::string crossMwmDataBuffer;
      crossMwmDataReader->ReadAsString(crossMwmDataBuffer);
      base::Json crossMwmData(crossMwmDataBuffer.data());

      json_t const * crossMwmJsonData = nullptr;
      FromJSONObjectOptionalField(crossMwmData.get(), countryName, crossMwmJsonData);
      if (crossMwmJsonData)
      {
        double leapSpeed = FromJSONObject<double>(crossMwmJsonData, "leapspeed");
        if (leapSpeed > 0.0)
          data.SetLeapWeightSpeed(leapSpeed);
      }
    }
    catch (FileAbsentException const & e)
    {
      LOG(LERROR, ("Error missing file", LEAP_SPEEDS_FILE, ":", e.Msg()));
      return false;
    }
    catch (Reader::Exception const & e)
    {
      LOG(LERROR, ("Error reading", LEAP_SPEEDS_FILE, ":", e.Msg()));
      return false;
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LERROR, ("Error parsing JSON in", LEAP_SPEEDS_FILE, ":", e.Msg()));
      return false;
    }
  }

  try
  {
    auto reader = GetPlatform().GetReader(COUNTRIES_META_FILE);
    std::string buffer;
    reader->ReadAsString(buffer);
    base::Json root(buffer.data());

    json_t const * jsonData = nullptr;
    FromJSONObjectOptionalField(root.get(), countryName, jsonData);
    if (!jsonData)
    {
      return false;
    }

    std::vector<std::string> languages;
    FromJSONObjectOptionalField(jsonData, "languages", languages);
    if (!languages.empty())
      data.SetLanguages(languages);

    std::string driving;
    FromJSONObjectOptionalField(jsonData, "driving", driving);
    if (driving == "l" || driving == "r")
      data.Set(RegionData::Type::RD_DRIVING, driving);

    std::string timezone;
    FromJSONObjectOptionalField(jsonData, "timezone", timezone);
    if (!timezone.empty())
      data.Set(RegionData::Type::RD_TIMEZONE, timezone);

    bool allow_housenames;
    FromJSONObjectOptionalField(jsonData, "housenames", allow_housenames);
    if (allow_housenames)
      data.Set(RegionData::Type::RD_ALLOW_HOUSENAMES, "y");

    {
      json_t const * phJson = nullptr;
      FromJSONObjectOptionalField(jsonData, "public_holidays", phJson);
      if (phJson && json_is_object(phJson))
      {
        feature::RegionData::THolidayTimestampSet holidays;
        feature::RegionData::THolidayNamesMap holidayNames;

        char const * key = nullptr;
        json_t * value = nullptr;
        size_t holidaysCount = 0;

        json_object_foreach(const_cast<json_t *>(phJson), key, value)
        {
          // key: "YYYY-MM-DD", value: "Holiday Name"
          try
          {
            time_t t = ParsePublicHolidayDate(key);
            holidays.insert(t);
            
            if (json_is_string(value))
            {
              char const * name = json_string_value(value);
              if (name && *name)
                holidayNames[t] = std::string(name);
            }
            
            holidaysCount++;
          }
          catch (base::Json::Exception const &)
          {
            LOG(LWARNING, ("Skipping invalid public holiday in", countryName, ":", key));
          }
        }

        if (!holidays.empty())
        {
          data.SetPublicHolidayTimestamps(std::move(holidays));
          if (!holidayNames.empty())
            data.SetPublicHolidayNames(holidayNames);
        }
      }
    }

    // Public holidays: an array of arrays of [string/number, number].
    // See https://github.com/opening-hours/opening_hours.js/blob/master/docs/holidays.md
    std::vector<json_t *> holidays;
    FromJSONObjectOptionalField(jsonData, "holidays", holidays);
    for (json_t * holiday : holidays)
    {
      if (!json_is_array(holiday) || json_array_size(holiday) != 2)
        MYTHROW(base::Json::Exception, ("Holiday must be an array of two elements in", countryName));
      json_t * reference = json_array_get(holiday, 0);
      int8_t refId = 0;
      if (json_is_integer(reference))
      {
        refId = json_integer_value(reference);
      }
      else if (json_is_string(reference))
      {
        refId = ParseHolidayReference(std::string(json_string_value(reference)));
      }
      else
      {
        MYTHROW(base::Json::Exception,
                ("Holiday month reference should be either a std::string or a number in", countryName));
      }

      if (refId <= 0)
        MYTHROW(base::Json::Exception, ("Incorrect month reference in", countryName));
      if (!json_is_integer(json_array_get(holiday, 1)))
        MYTHROW(base::Json::Exception, ("Holiday day offset should be a number in", countryName));
      data.AddPublicHoliday(refId, json_integer_value(json_array_get(holiday, 1)));
    }

    // TODO(@zverik): Implement formats reading when decided on data types.

    return true;
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error reading", COUNTRIES_META_FILE, ":", e.Msg()));
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, ("Error parsing JSON in", COUNTRIES_META_FILE, ":", e.Msg()));
  }
  return false;
}

bool ReadRegionData(std::string const & countryName, RegionData & data)
{
  // When there is a match for a complete countryName, simply relay the call.
  if (ReadRegionDataImpl(countryName, data))
  {
    LOG(LINFO, ("ReadRegionData: Matched full country name:", countryName));
    return true;
  }

  // If not, cut parts of a country name from the tail. E.g. "Russia_Moscow" -> "Russia".
  auto p = countryName.find_last_of('_');
  while (p != std::string::npos)
  {
    std::string const partialName = countryName.substr(0, p);
    if (ReadRegionDataImpl(partialName, data))
    {
      LOG(LINFO, ("ReadRegionData: Matched partial country name:", partialName, "from:", countryName));
      return true;
    }
    p = p > 0 ? countryName.find_last_of('_', p - 1) : std::string::npos;
  }
  LOG(LWARNING, ("ReadRegionData: No match found for country:", countryName));
  return false;
}
}  // namespace feature

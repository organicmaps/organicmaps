#include "region_meta.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"
#include "timezone/serdes.hpp"

#include "cppjansson/cppjansson.hpp"

#include <vector>

namespace feature
{
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

om::tz::TimeZone GetTimeZone(std::string const & tzName)
{
  return om::tz::GetTimeZoneDb().timezones.at(tzName);
}

void ReadEntryImpl(std::string_view keyName, json_t const * jsonData, RegionData & data);

void ReadRegionDataImpl(std::string const & regionID, RegionData & data)
{
  /// @todo How LEAP_SPEEDS_FILE was generated before? It's always absent now.
  if (Platform::IsFileExistsByFullPath(LEAP_SPEEDS_FILE))
  {
    auto crossMwmDataReader = GetPlatform().GetReader(LEAP_SPEEDS_FILE);
    std::string crossMwmDataBuffer;
    crossMwmDataReader->ReadAsString(crossMwmDataBuffer);
    base::Json crossMwmData(crossMwmDataBuffer.data());

    json_t const * crossMwmJsonData = nullptr;
    FromJSONObjectOptionalField(crossMwmData.get(), regionID, crossMwmJsonData);
    if (crossMwmJsonData)
    {
      double leapSpeed = FromJSONObject<double>(crossMwmJsonData, "leapspeed");
      if (leapSpeed > 0.0)
        data.SetLeapWeightSpeed(leapSpeed);
    }
  }

  auto reader = GetPlatform().GetReader(COUNTRIES_META_FILE);
  std::string buffer;
  reader->ReadAsString(buffer);
  base::Json root(buffer.data());

  json_t const * jsonData = nullptr;
  FromJSONObjectOptionalField(root.get(), regionID, jsonData);
  if (!jsonData)
    return;

  ReadEntryImpl(regionID, jsonData, data);
}

void ReadEntryImpl(std::string_view keyName, json_t const * jsonData, RegionData & data)
{
  std::vector<std::string> languages;
  FromJSONObjectOptionalField(jsonData, "languages", languages);
  if (!languages.empty())
    data.SetLanguages(languages);

  std::string driving;
  FromJSONObjectOptionalField(jsonData, "driving", driving);
  if (driving == "l" || driving == "r")
    data.Set(RegionData::Type::RD_DRIVING, driving);
  else
    CHECK(driving.empty(), (driving));

  std::string timezone;
  FromJSONObjectOptionalField(jsonData, "timezone", timezone);
  if (!timezone.empty())
  {
    std::string tz;
    CHECK_EQUAL(om::tz::Serialize(GetTimeZone(timezone), tz), om::tz::SerializationError::OK, (timezone));
    data.Set(RegionData::Type::RD_TIMEZONE, tz);
  }

  bool allow_housenames;
  FromJSONObjectOptionalField(jsonData, "housenames", allow_housenames);
  if (allow_housenames)
    data.Set(RegionData::Type::RD_ALLOW_HOUSENAMES, "y");

  // Public holidays: an array of arrays of [string/number, number].
  // See https://github.com/opening-hours/opening_hours.js/blob/master/docs/holidays.md
  std::vector<json_t *> holidays;
  FromJSONObjectOptionalField(jsonData, "holidays", holidays);
  for (json_t * holiday : holidays)
  {
    if (!json_is_array(holiday) || json_array_size(holiday) != 2)
      MYTHROW(base::Json::Exception, ("Holiday must be an array of two elements in", keyName));
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
              ("Holiday month reference should be either a std::string or a number in", keyName));
    }

    if (refId <= 0)
      MYTHROW(base::Json::Exception, ("Incorrect month reference in", keyName));
    if (!json_is_integer(json_array_get(holiday, 1)))
      MYTHROW(base::Json::Exception, ("Holiday day offset should be a number in", keyName));
    data.AddPublicHoliday(refId, json_integer_value(json_array_get(holiday, 1)));
  }
}

template <class TString>
bool ParentRegionID(TString & s)
{
  auto n = s.find_last_of('_');
  if (n == std::string::npos)
    return false;
  s = s.substr(0, n);
  return true;
}
}  // namespace

void ReadRegionData(std::string regionID, RegionData & data)
{
  do
  {
    ReadRegionDataImpl(regionID, data);
    if (!ParentRegionID(regionID))
      return;
  }
  while (true);
}

RegionData const * AllRegionsData::Get(std::string_view regionID) const
{
  do
  {
    auto it = m_cont.find(regionID);
    if (it != m_cont.end())
      return &it->second;

    if (!ParentRegionID(regionID))
      return nullptr;
  }
  while (true);
}

void AllRegionsData::Finish()
{
  for (auto & [country, data] : m_cont)
  {
    std::string_view key = country;
    while (true)
    {
      if (!ParentRegionID(key))
        break;

      auto it = m_cont.find(key);
      if (it != m_cont.end())
        data.MergeFrom(it->second);
    }
  }
}

void ReadAllRegions(AllRegionsData & allData)
{
  auto reader = GetPlatform().GetReader(COUNTRIES_META_FILE);
  std::string buffer;
  reader->ReadAsString(buffer);
  base::Json root(buffer.data());

  char const * key;
  json_t * value;
  json_object_foreach(root.get(), key, value)
  {
    RegionData data;
    ReadEntryImpl(key, value, data);
    CHECK(allData.m_cont.emplace(key, std::move(data)).second, (key));
  }

  allData.Finish();
}

}  // namespace feature

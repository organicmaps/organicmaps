#include "region_meta.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"
#include "timezone/serdes.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include "defines.hpp"

#include <glaze/json.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace feature
{
namespace region_meta_json
{
using JsonValue = glz::generic_u64;

struct Leap
{
  double leapspeed = 0.0;
};

struct Region
{
  std::vector<std::string> languages;
  std::string driving;
  std::string timezone;
  bool housenames = false;
  std::vector<JsonValue> holidays;
};

using Leaps = std::unordered_map<std::string, Leap>;
using Regions = std::unordered_map<std::string, Region>;
}  // namespace region_meta_json

namespace
{
DECLARE_EXCEPTION(RegionMetaJsonException, RootException);

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

om::tz::TimeZone const & GetTimeZone(std::string const & tzName)
{
  return om::tz::TimeZoneDb::Instance().GetTZ(tzName);
}

template <typename T>
T ReadJson(std::string const & buffer, char const * filename)
{
  T result;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(result, buffer); error)
    MYTHROW(RegionMetaJsonException, ("Cannot parse", filename, glz::format_error(error, buffer)));

  return result;
}

std::optional<int64_t> GetJsonInteger(region_meta_json::JsonValue const & value)
{
  if (auto const * signedValue = value.get_if<int64_t>())
    return *signedValue;

  if (auto const * unsignedValue = value.get_if<uint64_t>())
  {
    if (*unsignedValue <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
      return static_cast<int64_t>(*unsignedValue);
  }

  return std::nullopt;
}

int8_t ToInt8(int64_t value, std::string_view keyName, char const * message)
{
  if (value < std::numeric_limits<int8_t>::min() || value > std::numeric_limits<int8_t>::max())
    MYTHROW(RegionMetaJsonException, (message, "is out of range in", std::string(keyName)));

  return static_cast<int8_t>(value);
}

void ReadPublicHoliday(std::string_view keyName, region_meta_json::JsonValue const & holiday, RegionData & data)
{
  auto const * items = holiday.get_if<region_meta_json::JsonValue::array_t>();
  if (items == nullptr || items->size() != 2)
    MYTHROW(RegionMetaJsonException, ("Holiday must be an array of two elements in", std::string(keyName)));

  auto const & reference = (*items)[0];
  int8_t refId = 0;
  if (auto const numericReference = GetJsonInteger(reference))
  {
    refId = ToInt8(*numericReference, keyName, "Holiday month reference");
  }
  else if (reference.is_string())
  {
    refId = ParseHolidayReference(reference.get_string());
  }
  else
  {
    MYTHROW(RegionMetaJsonException,
            ("Holiday month reference should be either a std::string or a number in", std::string(keyName)));
  }

  if (refId <= 0)
    MYTHROW(RegionMetaJsonException, ("Incorrect month reference in", std::string(keyName)));

  auto const offset = GetJsonInteger((*items)[1]);
  if (!offset)
    MYTHROW(RegionMetaJsonException, ("Holiday day offset should be a number in", std::string(keyName)));

  data.AddPublicHoliday(refId, ToInt8(*offset, keyName, "Holiday day offset"));
}

void ReadEntryImpl(std::string_view keyName, region_meta_json::Region const & jsonData, RegionData & data);

void ReadRegionDataImpl(std::string const & regionID, RegionData & data)
{
  /// @todo How LEAP_SPEEDS_FILE was generated before? It's always absent now.
  if (Platform::IsFileExistsByFullPath(LEAP_SPEEDS_FILE))
  {
    auto crossMwmDataReader = GetPlatform().GetReader(LEAP_SPEEDS_FILE);
    std::string crossMwmDataBuffer;
    crossMwmDataReader->ReadAsString(crossMwmDataBuffer);
    auto const crossMwmData = ReadJson<region_meta_json::Leaps>(crossMwmDataBuffer, LEAP_SPEEDS_FILE);

    auto const it = crossMwmData.find(regionID);
    if (it != crossMwmData.end() && it->second.leapspeed > 0.0)
      data.SetLeapWeightSpeed(it->second.leapspeed);
  }

  auto reader = GetPlatform().GetReader(COUNTRIES_META_FILE);
  std::string buffer;
  reader->ReadAsString(buffer);
  auto const root = ReadJson<region_meta_json::Regions>(buffer, COUNTRIES_META_FILE);

  auto const it = root.find(regionID);
  if (it == root.end())
    return;

  ReadEntryImpl(regionID, it->second, data);
}

void ReadEntryImpl(std::string_view keyName, region_meta_json::Region const & jsonData, RegionData & data)
{
  if (!jsonData.languages.empty())
    data.SetLanguages(jsonData.languages);

  if (jsonData.driving == "l" || jsonData.driving == "r")
    data.Set(RegionData::Type::RD_DRIVING, jsonData.driving);
  else
    CHECK(jsonData.driving.empty(), (jsonData.driving));

  if (!jsonData.timezone.empty())
  {
    auto res = om::tz::Serialize(GetTimeZone(jsonData.timezone));
    CHECK(res, (jsonData.timezone));
    data.Set(RegionData::Type::RD_TIMEZONE, std::move(res.value()));
  }

  if (jsonData.housenames)
    data.Set(RegionData::Type::RD_ALLOW_HOUSENAMES, "y");

  // Public holidays: an array of arrays of [string/number, number].
  // See https://github.com/opening-hours/opening_hours.js/blob/master/docs/holidays.md
  for (auto const & holiday : jsonData.holidays)
    ReadPublicHoliday(keyName, holiday, data);
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
  auto const root = ReadJson<region_meta_json::Regions>(buffer, COUNTRIES_META_FILE);
  for (auto const & [key, value] : root)
  {
    RegionData data;
    ReadEntryImpl(key, value, data);
    CHECK(allData.m_cont.emplace(key, std::move(data)).second, (key));
  }

  allData.Finish();
}

}  // namespace feature

#include "generator/osm2meta.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"

#include "coding/url.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace
{

constexpr char const * kOSMMultivalueDelimiter = ";";

// https://en.wikipedia.org/wiki/List_of_tallest_buildings_in_the_world
auto constexpr kMaxBuildingLevelsInTheWorld = 167;
auto constexpr kMinBuildingLevel = -6;

template <class T>
void RemoveDuplicatesAndKeepOrder(vector<T> & vec)
{
  unordered_set<T> seen;
  auto const predicate = [&seen](T const & value)
  {
    if (seen.find(value) != seen.end())
      return true;
    seen.insert(value);
    return false;
  };
  vec.erase(remove_if(vec.begin(), vec.end(), predicate), vec.end());
}

// Also filters out duplicates.
class MultivalueCollector
{
public:
  void operator()(string const & value)
  {
    if (value.empty() || value == kOSMMultivalueDelimiter)
      return;
    m_values.push_back(value);
  }
  string GetString()
  {
    if (m_values.empty())
      return string();

    RemoveDuplicatesAndKeepOrder(m_values);
    return strings::JoinStrings(m_values, kOSMMultivalueDelimiter);
  }
private:
  vector<string> m_values;
};

bool IsNoNameNoAddressBuilding(FeatureParams const & params)
{
  static uint32_t const buildingType = classif().GetTypeByPath({"building"});
  return params.m_types.size() == 1 && params.m_types[0] == buildingType &&
         params.house.Get().empty() && params.name.IsEmpty();
}
}  // namespace

string MetadataTagProcessorImpl::ValidateAndFormat_stars(string const & v) const
{
  if (v.empty())
    return string();

  // We are accepting stars from 1 to 7.
  if (v[0] <= '0' || v[0] > '7')
    return string();

  // Ignore numbers large then 9.
  if (v.size() > 1 && ::isdigit(v[1]))
    return string();

  return string(1, v[0]);
}

string MetadataTagProcessorImpl::ValidateAndFormat_operator(string const & v) const
{
  auto const & t = m_params.m_types;
  if (ftypes::IsATMChecker::Instance()(t) ||
      ftypes::IsPaymentTerminalChecker::Instance()(t) ||
      ftypes::IsMoneyExchangeChecker::Instance()(t) ||
      ftypes::IsFuelStationChecker::Instance()(t) ||
      ftypes::IsRecyclingCentreChecker::Instance()(t) ||
      ftypes::IsPostOfficeChecker::Instance()(t) ||
      ftypes::IsCarSharingChecker::Instance()(t) ||
      ftypes::IsCarRentalChecker::Instance()(t) ||
      ftypes::IsBicycleRentalChecker::Instance()(t))
  {
    return v;
  }

  return {};
}

string MetadataTagProcessorImpl::ValidateAndFormat_url(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_phone(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_opening_hours(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_ele(string const & v) const
{
  if (IsNoNameNoAddressBuilding(m_params))
    return {};

  return measurement_utils::OSMDistanceToMetersString(v);
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes_forward(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes_backward(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_email(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_postcode(string const & v) const { return v; }

string MetadataTagProcessorImpl::ValidateAndFormat_flats(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_internet(string v) const
{
  // TODO(AlexZ): Reuse/synchronize this code with MapObject::SetInternet().
  strings::AsciiToLower(v);
  if (v == "wlan" || v == "wired" || v == "yes" || v == "no")
    return v;
  // Process wifi=free tag.
  if (v == "free")
    return "wlan";
  return {};
}

string MetadataTagProcessorImpl::ValidateAndFormat_height(string const & v) const
{
  return measurement_utils::OSMDistanceToMetersString(v, false /*supportZeroAndNegativeValues*/, 1);
}

string MetadataTagProcessorImpl::ValidateAndFormat_building_levels(string v) const
{
  // Some mappers use full width unicode digits. We can handle that.
  strings::NormalizeDigits(v);
  char * stop;
  char const * s = v.c_str();
  double const levels = strtod(s, &stop);
  if (s != stop && isfinite(levels) && levels >= 0 && levels <= kMaxBuildingLevelsInTheWorld)
    return strings::to_string_dac(levels, 1);

  return {};
}

string MetadataTagProcessorImpl::ValidateAndFormat_level(string v) const
{
  // Some mappers use full width unicode digits. We can handle that.
  strings::NormalizeDigits(v);
  char * stop;
  char const * s = v.c_str();
  double const levels = strtod(s, &stop);
  if (s != stop && isfinite(levels) && levels >= kMinBuildingLevel &&
      levels <= kMaxBuildingLevelsInTheWorld)
  {
    return strings::to_string(levels);
  }

  return {};
}

string MetadataTagProcessorImpl::ValidateAndFormat_denomination(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_wikipedia(string v) const
{
  strings::Trim(v);
  // Normalize by converting full URL to "lang:title" if necessary
  // (some OSMers do not follow standard for wikipedia tag).
  static string const base = ".wikipedia.org/wiki/";
  auto const baseIndex = v.find(base);
  if (baseIndex != string::npos)
  {
    auto const baseSize = base.size();
    // Do not allow urls without article name after /wiki/.
    if (v.size() > baseIndex + baseSize)
    {
      auto const slashIndex = v.rfind('/', baseIndex);
      if (slashIndex != string::npos && slashIndex + 1 != baseIndex)
      {
        // Normalize article title according to OSM standards.
        string title = url::UrlDecode(v.substr(baseIndex + baseSize));
        replace(title.begin(), title.end(), '_', ' ');
        return v.substr(slashIndex + 1, baseIndex - slashIndex - 1) + ":" + title;
      }
    }
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Standard case: "lang:Article Name With Spaces".
  // Language and article are at least 2 chars each.
  auto const colonIndex = v.find(':');
  if (colonIndex == string::npos || colonIndex < 2 || colonIndex + 2 > v.size())
  {
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Check if it's not a random/invalid link.
  if (v.find("//") != string::npos || v.find(".org") != string::npos)
  {
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Normalize to OSM standards.
  string normalized(v);
  replace(normalized.begin() + colonIndex, normalized.end(), '_', ' ');
  return normalized;
}

string MetadataTagProcessorImpl::ValidateAndFormat_airport_iata(string const & v) const
{
  if (!ftypes::IsAirportChecker::Instance()(m_params.m_types))
    return {};

  if (v.size() != 3)
    return {};

  auto str = v;
  for (auto & c : str)
  {
    if (!isalpha(c))
      return {};
    c = toupper(c);
  }
  return str;
}

string MetadataTagProcessorImpl::ValidateAndFormat_duration(string const & v) const
{
  if (!ftypes::IsFerryChecker::Instance()(m_params.m_types))
    return {};

  auto const format = [](double hours) -> string {
    if (base::AlmostEqualAbs(hours, 0.0, 1e-5))
      return {};

    stringstream ss;
    ss << setprecision(5);
    ss << hours;
    return ss.str();
  };

  auto const readNumber = [&v](size_t & pos) -> optional<uint32_t> {
    uint32_t number = 0;
    size_t const startPos = pos;
    while (pos < v.size() && isdigit(v[pos]))
    {
      number *= 10;
      number += v[pos] - '0';
      ++pos;
    }

    if (startPos == pos)
      return {};

    return {number};
  };

  auto const convert = [](char type, uint32_t number) -> optional<double> {
    switch (type)
    {
    case 'H': return number;
    case 'M': return number / 60.0;
    case 'S': return number / 3600.0;
    }

    return {};
  };

  if (v.empty())
    return {};

  double hours = 0.0;
  size_t pos = 0;
  optional<uint32_t> op;

  if (strings::StartsWith(v, "PT"))
  {
    if (v.size() < 4)
      return {};

    pos = 2;
    while (pos < v.size() && (op = readNumber(pos)))
    {
      if (pos >= v.size())
        return {};

      char const type = v[pos];
      auto const addHours = convert(type, *op);
      if (addHours)
        hours += *addHours;
      else
        return {};

      ++pos;
    }

    if (!op)
      return {};

    return format(hours);
  }

  // "hh:mm:ss" or just "mm"
  vector<uint32_t> numbers;
  while (pos < v.size() && (op = readNumber(pos)))
  {
    numbers.emplace_back(*op);
    if (pos >= v.size())
      break;

    if (v[pos] != ':')
      return {};

    ++pos;
  }

  if (numbers.size() > 3 || !op)
    return {};

  if (numbers.size() == 1)
    return format(numbers.back() / 60.0);

  double pow = 1.0;
  for (auto number : numbers)
  {
    hours += number / pow;
    pow *= 60.0;
  }

  return format(hours);
}


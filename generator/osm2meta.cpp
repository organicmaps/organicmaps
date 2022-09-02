#include "generator/osm2meta.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/editable_map_object.hpp"

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
using osm::EditableMapObject;

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

bool Prefix2Double(string const & str, double & d)
{
  char * stop;
  char const * s = str.c_str();
  d = strtod(s, &stop);
  return (s != stop && strings::is_finite(d));
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

string MetadataTagProcessorImpl::ValidateAndFormat_destination(string const & v) const
{
  // Normalization. "a1 a2;b1-b2;  c,d ;e,;f;  ;g" -> "a1 a2; b1-b2; c; d; e; f; g"
  string r;
  strings::Tokenize(v, ";,", [&](std::string_view d)
  {
    strings::Trim(d);
    if (d.empty())
      return;
    if (!r.empty())
      r += "; ";
    r.append(d);
  });
  return r;
}

string MetadataTagProcessorImpl::ValidateAndFormat_destination_ref(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_junction_ref(string const & v) const
{
  return v;
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
  strings::AsciiToLower(v);
  if (v == "wlan" || v == "wired" || v == "terminal" || v == "yes" || v == "no")
    return v;
  // Process additional top tags.
  if (v == "free" || v == "wifi" || v == "public")
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
  double levels;
  if (Prefix2Double(v, levels) && levels >= 0 && levels <= kMaxBuildingLevelsInTheWorld)
    return strings::to_string_dac(levels, 1);

  return {};
}

string MetadataTagProcessorImpl::ValidateAndFormat_level(string v) const
{
  // Some mappers use full width unicode digits. We can handle that.
  strings::NormalizeDigits(v);
  double levels;
  if (Prefix2Double(v, levels) && levels >= kMinBuildingLevel && levels <= kMaxBuildingLevelsInTheWorld)
    return strings::to_string(levels);

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

string MetadataTagProcessorImpl::ValidateAndFormat_wikimedia_commons(string v) const
{

    // Putting the full wikimedia url to this tag is incorrect according to:
    // https://wiki.openstreetmap.org/wiki/Key:wikimedia_commons
    // But it happens often enough that we should guard against it.
    strings::ReplaceFirst(v, "https://commons.wikimedia.org/wiki/", "");
    strings::ReplaceFirst(v, "https://commons.m.wikimedia.org/wiki/", "");

    if(strings::StartsWith(v, "File:") || strings::StartsWith(v, "Category:"))
    {
        return v;
    }
    else
    {
        LOG(LDEBUG, ("Invalid Wikimedia Commons tag value:", v));
        return string();
    }
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
  if (!ftypes::IsWayWithDurationChecker::Instance()(m_params.m_types))
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


MetadataTagProcessor::~MetadataTagProcessor()
{
  if (!m_description.IsEmpty())
    m_params.GetMetadata().Set(feature::Metadata::FMD_DESCRIPTION, m_description.GetBuffer());
}

void MetadataTagProcessor::operator()(std::string const & k, std::string const & v)
{
  if (v.empty())
    return;

  using feature::Metadata;
  Metadata & md = m_params.GetMetadata();

  if (strings::StartsWith(k, "description"))
  {
    // Process description tags.
    int8_t lang = StringUtf8Multilang::kDefaultCode;
    size_t const i = k.find(':');
    if (i != std::string::npos)
    {
      int8_t const l = StringUtf8Multilang::GetLangIndex(k.substr(i+1));
      if (l != StringUtf8Multilang::kUnsupportedLanguageCode)
        lang = l;
    }

    m_description.AddString(lang, v);
    return;
  }

  Metadata::EType mdType;
  if (!Metadata::TypeFromString(k, mdType))
    return;

  std::string valid;
  switch (mdType)
  {
  case Metadata::FMD_OPEN_HOURS: valid = ValidateAndFormat_opening_hours(v); break;
  case Metadata::FMD_FAX_NUMBER:  // The same validator as for phone.
  case Metadata::FMD_PHONE_NUMBER: valid = ValidateAndFormat_phone(v); break;
  case Metadata::FMD_STARS: valid = ValidateAndFormat_stars(v); break;
  case Metadata::FMD_OPERATOR: valid = ValidateAndFormat_operator(v); break;
  case Metadata::FMD_WEBSITE: valid = ValidateAndFormat_url(v); break;
  case Metadata::FMD_CONTACT_FACEBOOK: valid = osm::ValidateAndFormat_facebook(v); break;
  case Metadata::FMD_CONTACT_INSTAGRAM: valid = osm::ValidateAndFormat_instagram(v); break;
  case Metadata::FMD_CONTACT_TWITTER: valid = osm::ValidateAndFormat_twitter(v); break;
  case Metadata::FMD_CONTACT_VK: valid = osm::ValidateAndFormat_vk(v); break;
  case Metadata::FMD_CONTACT_LINE: valid = osm::ValidateAndFormat_contactLine(v); break;
  case Metadata::FMD_INTERNET: valid = ValidateAndFormat_internet(v); break;
  case Metadata::FMD_ELE: valid = ValidateAndFormat_ele(v); break;
  case Metadata::FMD_DESTINATION: valid = ValidateAndFormat_destination(v); break;
  case Metadata::FMD_DESTINATION_REF: valid = ValidateAndFormat_destination_ref(v); break;
  case Metadata::FMD_JUNCTION_REF: valid = ValidateAndFormat_junction_ref(v); break;
  case Metadata::FMD_TURN_LANES: valid = ValidateAndFormat_turn_lanes(v); break;
  case Metadata::FMD_TURN_LANES_FORWARD: valid = ValidateAndFormat_turn_lanes_forward(v); break;
  case Metadata::FMD_TURN_LANES_BACKWARD: valid = ValidateAndFormat_turn_lanes_backward(v); break;
  case Metadata::FMD_EMAIL: valid = ValidateAndFormat_email(v); break;
  case Metadata::FMD_POSTCODE: valid = ValidateAndFormat_postcode(v); break;
  case Metadata::FMD_WIKIPEDIA: valid = ValidateAndFormat_wikipedia(v); break;
  case Metadata::FMD_WIKIMEDIA_COMMONS: valid = ValidateAndFormat_wikimedia_commons(v); break;
  case Metadata::FMD_FLATS: valid = ValidateAndFormat_flats(v); break;
  case Metadata::FMD_MIN_HEIGHT:  // The same validator as for height.
  case Metadata::FMD_HEIGHT: valid = ValidateAndFormat_height(v); break;
  case Metadata::FMD_DENOMINATION: valid = ValidateAndFormat_denomination(v); break;
  case Metadata::FMD_BUILDING_MIN_LEVEL:  // The same validator as for building_levels.
  case Metadata::FMD_BUILDING_LEVELS: valid = ValidateAndFormat_building_levels(v); break;
  case Metadata::FMD_LEVEL: valid = ValidateAndFormat_level(v); break;
  case Metadata::FMD_AIRPORT_IATA: valid = ValidateAndFormat_airport_iata(v); break;
  case Metadata::FMD_DURATION: valid = ValidateAndFormat_duration(v); break;
  // Metadata types we do not get from OSM.
  case Metadata::FMD_CUISINE:
  case Metadata::FMD_BRAND:
  case Metadata::FMD_DESCRIPTION:   // processed separately
  case Metadata::FMD_TEST_ID:
  case Metadata::FMD_COUNT: CHECK(false, (mdType, "should not be parsed from OSM."));
  }

  md.Set(mdType, valid);
}

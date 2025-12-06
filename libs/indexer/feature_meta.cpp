#include "indexer/feature_meta.hpp"
#include "custom_keyvalue.hpp"

#include "std/target_os.hpp"

namespace feature
{
using namespace std;

namespace
{
char constexpr const * kBaseWikiUrl =
#ifdef OMIM_OS_MOBILE
    ".m.wikipedia.org/wiki/";
#else
    ".wikipedia.org/wiki/";
#endif

char constexpr const * kBaseCommonsUrl =
#ifdef OMIM_OS_MOBILE
    "https://commons.m.wikimedia.org/wiki/";
#else
    "https://commons.wikimedia.org/wiki/";
#endif
}  // namespace

std::string_view MetadataBase::Get(uint8_t type) const
{
  std::string_view sv;
  auto const it = m_metadata.find(type);
  if (it != m_metadata.end())
  {
    sv = it->second;
    ASSERT(!sv.empty(), ());
  }
  return sv;
}

std::string_view MetadataBase::Set(uint8_t type, std::string value)
{
  std::string_view sv;

  if (value.empty())
    m_metadata.erase(type);
  else
  {
    auto & res = m_metadata[type];
    res = std::move(value);
    sv = res;
  }

  return sv;
}

string Metadata::ToWikiURL(std::string v)
{
  auto const colon = v.find(':');
  if (colon == string::npos)
    return v;

  // Spaces, % and ? characters should be corrected to form a valid URL's path.
  // Standard percent encoding also encodes other characters like (), which lead to an unnecessary HTTP redirection.
  for (auto i = colon; i < v.size(); ++i)
  {
    auto & c = v[i];
    if (c == ' ')
      c = '_';
    else if (c == '%')
      v.insert(i + 1, "25");  // % => %25
    else if (c == '?')
    {
      c = '%';
      v.insert(i + 1, "3F");  // ? => %3F
    }
  }

  // Trying to avoid redirects by constructing the right link.
  // TODO: Wikipedia article could be opened in a user's language, but need
  // generator level support to check for available article languages first.
  return "https://" + v.substr(0, colon) + kBaseWikiUrl + v.substr(colon + 1);
}

std::string Metadata::GetWikiURL() const
{
  return ToWikiURL(string(Get(FMD_WIKIPEDIA)));
}

string Metadata::ToWikimediaCommonsURL(std::string const & v)
{
  if (v.empty())
    return v;

  // Use the media viewer for single files
  if (v.starts_with("File:"))
    return kBaseCommonsUrl + v + "#/media/" + v;

  // or standard if it's a category
  return kBaseCommonsUrl + v;
}

// static
bool Metadata::TypeFromString(string_view k, Metadata::EType & outType)
{
  if (k == "opening_hours")
    outType = Metadata::FMD_OPEN_HOURS;
  else if (k == "phone" || k == "contact:phone" || k == "contact:mobile" || k == "mobile")
    outType = Metadata::FMD_PHONE_NUMBER;
  else if (k == "fax" || k == "contact:fax")
    outType = Metadata::EType::FMD_FAX_NUMBER;
  else if (k == "stars")
    outType = Metadata::FMD_STARS;
  else if (k.starts_with("operator"))
    outType = Metadata::FMD_OPERATOR;
  else if (k == "url" || k == "website" || k == "contact:website")
    outType = Metadata::FMD_WEBSITE;
  else if (k == "facebook" || k == "contact:facebook")
    outType = Metadata::FMD_CONTACT_FACEBOOK;
  else if (k == "instagram" || k == "contact:instagram")
    outType = Metadata::FMD_CONTACT_INSTAGRAM;
  else if (k == "twitter" || k == "contact:twitter")
    outType = Metadata::FMD_CONTACT_TWITTER;
  else if (k == "vk" || k == "contact:vk")
    outType = Metadata::FMD_CONTACT_VK;
  else if (k == "contact:line")
    outType = Metadata::FMD_CONTACT_LINE;
  else if (k == "internet_access" || k == "wifi")
    outType = Metadata::FMD_INTERNET;
  else if (k == "ele")
    outType = Metadata::FMD_ELE;
  else if (k == "destination")
    outType = Metadata::FMD_DESTINATION;
  else if (k == "destination:ref")
    outType = Metadata::FMD_DESTINATION_REF;
  else if (k == "junction:ref")
    outType = Metadata::FMD_JUNCTION_REF;
  else if (k == "turn:lanes")
    outType = Metadata::FMD_TURN_LANES;
  else if (k == "turn:lanes:forward")
    outType = Metadata::FMD_TURN_LANES_FORWARD;
  else if (k == "turn:lanes:backward")
    outType = Metadata::FMD_TURN_LANES_BACKWARD;
  else if (k == "email" || k == "contact:email")
    outType = Metadata::FMD_EMAIL;
  // Process only _main_ tag here, needed for editor ser/des. Actual postcode parsing happens in GetNameAndType.
  else if (k == "addr:postcode")
    outType = Metadata::FMD_POSTCODE;
  else if (k == "wikipedia")
    outType = Metadata::FMD_WIKIPEDIA;
  else if (k == "wikimedia_commons")
    outType = Metadata::FMD_WIKIMEDIA_COMMONS;
  else if (k == "addr:flats")
    outType = Metadata::FMD_FLATS;
  else if (k == "height")
    outType = Metadata::FMD_HEIGHT;
  else if (k == "min_height")
    outType = Metadata::FMD_MIN_HEIGHT;
  else if (k == "building:levels")
    outType = Metadata::FMD_BUILDING_LEVELS;
  else if (k == "building:min_level")
    outType = Metadata::FMD_BUILDING_MIN_LEVEL;
  else if (k == "denomination")
    outType = Metadata::FMD_DENOMINATION;
  else if (k == "level")
    outType = Metadata::FMD_LEVEL;
  else if (k == "iata")
    outType = Metadata::FMD_AIRPORT_IATA;
  else if (k.starts_with("brand"))
    outType = Metadata::FMD_BRAND;
  else if (k == "duration")
    outType = Metadata::FMD_DURATION;
  else if (k == "capacity")
    outType = Metadata::FMD_CAPACITY;
  else if (k == "local_ref")
    outType = Metadata::FMD_LOCAL_REF;
  else if (k == "drive_through")
    outType = Metadata::FMD_DRIVE_THROUGH;
  else if (k == "website:menu")
    outType = Metadata::FMD_WEBSITE_MENU;
  else if (k == "self_service")
    outType = Metadata::FMD_SELF_SERVICE;
  else if (k == "outdoor_seating")
    outType = Metadata::FMD_OUTDOOR_SEATING;
  else if (k == "network")
    outType = Metadata::FMD_NETWORK;
  else
    return false;

  return true;
}

void Metadata::ClearPOIAttribs()
{
  for (auto i = m_metadata.begin(); i != m_metadata.end();)
    if (i->first != Metadata::FMD_ELE && i->first != Metadata::FMD_POSTCODE && i->first != Metadata::FMD_FLATS &&
        i->first != Metadata::FMD_HEIGHT && i->first != Metadata::FMD_MIN_HEIGHT &&
        i->first != Metadata::FMD_BUILDING_LEVELS && i->first != Metadata::FMD_TEST_ID &&
        i->first != Metadata::FMD_BUILDING_MIN_LEVEL)
    {
      i = m_metadata.erase(i);
    }
    else
      ++i;
}

void RegionData::SetLanguages(vector<string> const & codes)
{
  string value;
  for (string const & code : codes)
  {
    int8_t const lang = StringUtf8Multilang::GetLangIndex(code);
    if (lang != StringUtf8Multilang::kUnsupportedLanguageCode)
      value.push_back(lang);
  }
  MetadataBase::Set(RegionData::Type::RD_LANGUAGES, value);
}

void RegionData::GetLanguages(vector<int8_t> & langs) const
{
  for (auto const lang : Get(RegionData::Type::RD_LANGUAGES))
    langs.push_back(lang);
}

bool RegionData::HasLanguage(int8_t const lang) const
{
  for (auto const lng : Get(RegionData::Type::RD_LANGUAGES))
    if (lng == lang)
      return true;
  return false;
}

bool RegionData::IsSingleLanguage(int8_t const lang) const
{
  auto const value = Get(RegionData::Type::RD_LANGUAGES);
  if (value.size() != 1)
    return false;
  return value.front() == lang;
}

void RegionData::AddPublicHoliday(int8_t month, int8_t offset)
{
  string value(Get(RegionData::Type::RD_PUBLIC_HOLIDAYS));
  value.push_back(month);
  value.push_back(offset);
  Set(RegionData::Type::RD_PUBLIC_HOLIDAYS, std::move(value));
}

void RegionData::SetPublicHolidayTimestamps(THolidayTimestampSet holidays)
{
  // Empty set → just clear meta value.
  if (holidays.empty())
  {
    Set(RegionData::Type::RD_PUBLIC_HOLIDAYS, std::string());
    return;
  }

  // Pack each time_t as 8 bytes into one string.
  std::string value;
  value.reserve(holidays.size() * sizeof(int64_t));

  for (time_t t : holidays)
  {
    int64_t v = static_cast<int64_t>(t);
    for (size_t i = 0; i < sizeof(v); ++i)
      value.push_back(static_cast<char>((v >> (i * 8)) & 0xFF));
  }

  Set(RegionData::Type::RD_PUBLIC_HOLIDAYS, std::move(value));
}

void RegionData::GetPublicHolidayTimestamps(THolidayTimestampSet & holidays) const
{
  holidays.clear();

  std::string_view value = Get(RegionData::Type::RD_PUBLIC_HOLIDAYS);
  LOG(LINFO, ("RegionData::GetPublicHolidayTimestamps: Value size:", value.size(), "bytes"));
  if (value.empty())
  {
    LOG(LINFO, ("RegionData::GetPublicHolidayTimestamps: No holidays in RegionData (empty value)"));
    return;
  }

  size_t const sz = value.size();
  if (sz % sizeof(int64_t) != 0)
  {
    ASSERT(false, ("Corrupted RD_PUBLIC_HOLIDAYS value"));
    return;
  }

  for (size_t offset = 0; offset < sz; offset += sizeof(int64_t))
  {
    int64_t v = 0;
    for (size_t i = 0; i < sizeof(v); ++i)
    {
      unsigned char byte = static_cast<unsigned char>(value[offset + i]);
      v |= static_cast<int64_t>(byte) << (i * 8);
    }
    holidays.insert(static_cast<time_t>(v));
  }
}

void RegionData::SetPublicHolidayNames(THolidayNamesMap const & names)
{
  if (names.empty())
  {
    Set(RegionData::Type::RD_PUBLIC_HOLIDAY_NAMES, std::string());
    return;
  }

  std::string value;
  for (auto const & [timestamp, name] : names)
  {
    value += std::to_string(timestamp);
    value += '|';
    value += name;
    value += '\n';
  }

  Set(RegionData::Type::RD_PUBLIC_HOLIDAY_NAMES, std::move(value));
}

void RegionData::GetPublicHolidayNames(THolidayNamesMap & names) const
{
  names.clear();

  std::string_view value = Get(RegionData::Type::RD_PUBLIC_HOLIDAY_NAMES);
  if (value.empty())
    return;

  std::string_view remaining = value;
  while (!remaining.empty())
  {
    size_t pipePos = remaining.find('|');
    if (pipePos == std::string_view::npos)
      break;

    size_t newlinePos = remaining.find('\n', pipePos);
    if (newlinePos == std::string_view::npos)
      newlinePos = remaining.size();

    std::string_view timestampStr = remaining.substr(0, pipePos);
    std::string_view nameStr = remaining.substr(pipePos + 1, newlinePos - pipePos - 1);

    try
    {
      time_t timestamp = static_cast<time_t>(std::stoll(std::string(timestampStr)));
      names[timestamp] = std::string(nameStr);
    }
    catch (std::exception const &)
    {
    }

    if (newlinePos < remaining.size())
      remaining = remaining.substr(newlinePos + 1);
    else
      break;
  }
}

// Warning: exact osm tag keys should be returned for valid enum values.
string ToString(Metadata::EType type)
{
  switch (type)
  {
  case Metadata::FMD_CUISINE: return "cuisine";
  case Metadata::FMD_OPEN_HOURS: return "opening_hours";
  case Metadata::FMD_PHONE_NUMBER: return "phone";
  case Metadata::FMD_FAX_NUMBER: return "fax";
  case Metadata::FMD_STARS: return "stars";
  case Metadata::FMD_OPERATOR: return "operator";
  case Metadata::FMD_WEBSITE: return "website";
  case Metadata::FMD_INTERNET: return "internet_access";
  case Metadata::FMD_ELE: return "ele";
  case Metadata::FMD_TURN_LANES: return "turn:lanes";
  case Metadata::FMD_TURN_LANES_FORWARD: return "turn:lanes:forward";
  case Metadata::FMD_TURN_LANES_BACKWARD: return "turn:lanes:backward";
  case Metadata::FMD_EMAIL: return "email";
  case Metadata::FMD_POSTCODE: return "addr:postcode";
  case Metadata::FMD_WIKIPEDIA: return "wikipedia";
  case Metadata::FMD_DESCRIPTION: return "description";
  case Metadata::FMD_FLATS: return "addr:flats";
  case Metadata::FMD_HEIGHT: return "height";
  case Metadata::FMD_MIN_HEIGHT: return "min_height";
  case Metadata::FMD_DENOMINATION: return "denomination";
  case Metadata::FMD_BUILDING_LEVELS: return "building:levels";
  case Metadata::FMD_TEST_ID: return "test_id";
  case Metadata::FMD_CUSTOM_IDS: return "custom_ids";
  case Metadata::FMD_PRICE_RATES: return "price_rates";
  case Metadata::FMD_RATINGS: return "ratings";
  case Metadata::FMD_EXTERNAL_URI: return "external_uri";
  case Metadata::FMD_LEVEL: return "level";
  case Metadata::FMD_AIRPORT_IATA: return "iata";
  case Metadata::FMD_BRAND: return "brand";
  case Metadata::FMD_DURATION: return "duration";
  case Metadata::FMD_CONTACT_FACEBOOK: return "contact:facebook";
  case Metadata::FMD_CONTACT_INSTAGRAM: return "contact:instagram";
  case Metadata::FMD_CONTACT_TWITTER: return "contact:twitter";
  case Metadata::FMD_CONTACT_VK: return "contact:vk";
  case Metadata::FMD_CONTACT_LINE: return "contact:line";
  case Metadata::FMD_DESTINATION: return "destination";
  case Metadata::FMD_DESTINATION_REF: return "destination:ref";
  case Metadata::FMD_JUNCTION_REF: return "junction:ref";
  case Metadata::FMD_BUILDING_MIN_LEVEL: return "building:min_level";
  case Metadata::FMD_WIKIMEDIA_COMMONS: return "wikimedia_commons";
  case Metadata::FMD_CAPACITY: return "capacity";
  case Metadata::FMD_WHEELCHAIR: return "wheelchair";
  case Metadata::FMD_LOCAL_REF: return "local_ref";
  case Metadata::FMD_DRIVE_THROUGH: return "drive_through";
  case Metadata::FMD_WEBSITE_MENU: return "website:menu";
  case Metadata::FMD_SELF_SERVICE: return "self_service";
  case Metadata::FMD_OUTDOOR_SEATING: return "outdoor_seating";
  case Metadata::FMD_NETWORK: return "network";
  case Metadata::FMD_COUNT: CHECK(false, ("FMD_COUNT can not be used as a type."));
  };

  return {};
}

string DebugPrint(Metadata const & metadata)
{
  bool first = true;
  std::string res = "Metadata [";
  for (uint8_t i = 0; i < static_cast<uint8_t>(Metadata::FMD_COUNT); ++i)
  {
    auto const t = static_cast<Metadata::EType>(i);
    auto const sv = metadata.Get(t);
    if (!sv.empty())
    {
      if (first)
        first = false;
      else
        res += "; ";

      res.append(DebugPrint(t)).append("=");
      switch (t)
      {
      case Metadata::FMD_DESCRIPTION: res += DebugPrint(StringUtf8Multilang::FromBuffer(std::string(sv))); break;
      case Metadata::FMD_CUSTOM_IDS:
      case Metadata::FMD_PRICE_RATES:
      case Metadata::FMD_RATINGS: res += DebugPrint(indexer::CustomKeyValue(sv)); break;
      default: res.append(sv); break;
      }
    }
  }
  res += "]";
  return res;
}

string DebugPrint(AddressData const & addressData)
{
  return string("AddressData { Street = \"").append(addressData.Get(AddressData::Type::Street)) + "\" }";
}
}  // namespace feature

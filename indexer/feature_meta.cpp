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
} // namespace

string Metadata::ToWikiURL(std::string v)
{
  auto const colon = v.find(':');
  if (colon == string::npos)
    return v;

  // Spaces and % sign should be replaced in urls.
  replace(v.begin() + colon, v.end(), ' ', '_');
  string::size_type percent, pos = colon;
  string const escapedPercent("%25");
  while ((percent = v.find('%', pos)) != string::npos)
  {
    v.replace(percent, 1, escapedPercent);
    pos = percent + escapedPercent.size();
  }

  // Trying to avoid redirects by constructing the right link.
  // TODO: Wikipedia article could be opened it a user's language, but need
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

  return "https://commons.wikimedia.org/wiki/" + v;
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
  else if (k == "operator")
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
  else if (k == "duration")
    outType = Metadata::FMD_DURATION;
  else
    return false;

  return true;
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
  {
    if (lng == lang)
      return true;
  }
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
      case Metadata::FMD_DESCRIPTION:
        res += DebugPrint(StringUtf8Multilang::FromBuffer(std::string(sv)));
        break;
      case Metadata::FMD_CUSTOM_IDS:
      case Metadata::FMD_PRICE_RATES:
      case Metadata::FMD_RATINGS:
        res += DebugPrint(indexer::CustomKeyValue(sv));
        break;
      default:
        res.append(sv);
        break;
      }
    }
  }
  res += "]";
  return res;
}

string DebugPrint(feature::AddressData const & addressData)
{
  return std::string("AddressData [")
          .append("Street = \"").append(addressData.Get(AddressData::Type::Street)).append("\"; ")
          .append("Postcode = \"").append(addressData.Get(AddressData::Type::Postcode)).append("\"]");
}
}  // namespace feature

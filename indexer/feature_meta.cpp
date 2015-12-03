#include "indexer/feature_meta.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"

namespace feature
{

namespace
{
char constexpr const * kBaseWikiUrl =
#ifdef OMIM_OS_MOBILE
    ".m.wikipedia.org/wiki/";
#else
    ".wikipedia.org/wiki/";
#endif
} // namespace

string Metadata::GetWikiURL() const
{
  string v = this->Get(FMD_WIKIPEDIA);
  if (v.empty())
    return v;

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

}  // namespace feature

string DebugPrint(feature::Metadata::EType type)
{
  using feature::Metadata;
  switch (type)
  {
  case Metadata::FMD_CUISINE: return "Cuisine";
  case Metadata::FMD_OPEN_HOURS: return "Open Hours";
  case Metadata::FMD_PHONE_NUMBER: return "Phone";
  case Metadata::FMD_FAX_NUMBER: return "Fax";
  case Metadata::FMD_STARS: return "Stars";
  case Metadata::FMD_OPERATOR: return "Operator";
  case Metadata::FMD_URL: return "URL";
  case Metadata::FMD_WEBSITE: return "Website";
  case Metadata::FMD_INTERNET: return "Internet";
  case Metadata::FMD_ELE: return "Elevation";
  case Metadata::FMD_TURN_LANES: return "Turn Lanes";
  case Metadata::FMD_TURN_LANES_FORWARD: return "Turn Lanes Forward";
  case Metadata::FMD_TURN_LANES_BACKWARD: return "Turn Lanes Backward";
  case Metadata::FMD_EMAIL: return "Email";
  case Metadata::FMD_POSTCODE: return "Postcode";
  case Metadata::FMD_WIKIPEDIA: return "Wikipedia";
  case Metadata::FMD_MAXSPEED: return "Maxspeed";
  case Metadata::FMD_FLATS: return "Flats";
  case Metadata::FMD_COUNT: CHECK(false, ("FMD_COUNT can not be used as a type."));
  };

  return string();
}

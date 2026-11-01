#include "map/share.hpp"

#include "ge0/url_generator.hpp"

#include "indexer/map_object.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"

#include <string_view>

namespace share
{
using feature::Metadata;

namespace
{
std::string EscapeHtml(std::string_view s)
{
  std::string out;
  out.reserve(s.size());
  for (char const c : s)
  {
    switch (c)
    {
    case '&': out += "&amp;"; break;
    case '<': out += "&lt;"; break;
    case '>': out += "&gt;"; break;
    case '"': out += "&quot;"; break;
    default: out += c;
    }
  }
  return out;
}

// OSM values occasionally carry an upper-case scheme ("HTTP://example.com"); schemes are
// case-insensitive, so match them that way instead of prepending a second scheme.
size_t UrlSchemeLength(std::string_view url)
{
  for (std::string_view const scheme : {"https://", "http://"})
    if (url.size() >= scheme.size() && strings::EqualAsciiNoCase(url.substr(0, scheme.size()), scheme))
      return scheme.size();
  return 0;
}

std::string_view StripUrlScheme(std::string_view url)
{
  return url.substr(UrlSchemeLength(url));
}

std::string AddUrlScheme(std::string_view url)
{
  if (UrlSchemeLength(url) != 0)
    return std::string{url};
  // OSM website tags commonly omit the scheme, but HTML anchors require an absolute URL.
  return "https://" + std::string{url};
}

// RFC 3966 forbids spaces in a tel: uri, while OSM phone numbers are usually written with them
// ("+49 30 1234"). Visual separators ("-.()") are allowed, so only whitespace is dropped.
std::string ToTelUri(std::string_view phone)
{
  std::string uri = "tel:";
  uri.reserve(uri.size() + phone.size());
  for (char const c : phone)
    if (!strings::IsASCIISpace(c))
      uri += c;
  return uri;
}

void AppendText(std::string & html, std::string_view text)
{
  html += EscapeHtml(text);
  html += "<br>\n";
}

void AppendAnchor(std::string & html, std::string_view href, std::string_view text)
{
  html += "<a href=\"";
  html += EscapeHtml(href);
  html += "\">";
  html += EscapeHtml(text);
  html += "</a><br>\n";
}

// Renders one allow-listed metadata field into the HTML body: phones/emails/sites become links,
// everything else is shown as text. Wiki values arrive already as URLs (see FillMetadata).
void AppendHtmlField(std::string & html, Metadata::EType type, std::string const & value)
{
  switch (type)
  {
  // OSM stores multiple values in one ";"-separated tag.
  case Metadata::FMD_PHONE_NUMBER:
    strings::TokenizeAndTrim(value, ";",
                             [&html](std::string_view phone) { AppendAnchor(html, ToTelUri(phone), phone); });
    break;
  case Metadata::FMD_EMAIL:
    strings::TokenizeAndTrim(
        value, ";", [&html](std::string_view email) { AppendAnchor(html, "mailto:" + std::string{email}, email); });
    break;
  case Metadata::FMD_WEBSITE:
  case Metadata::FMD_WEBSITE_MENU:
    strings::TokenizeAndTrim(
        value, ";", [&html](std::string_view site) { AppendAnchor(html, AddUrlScheme(site), StripUrlScheme(site)); });
    break;
  case Metadata::FMD_WIKIPEDIA: AppendAnchor(html, value, "Wikipedia"); break;
  default: AppendText(html, value); break;
  }
}
}  // namespace

void FillMetadata(Place & place, osm::MapObject const & obj)
{
  // The order AppendHtmlField renders them in.
  static constexpr Metadata::EType kFields[] = {Metadata::FMD_OPEN_HOURS,   Metadata::FMD_PHONE_NUMBER,
                                                Metadata::FMD_EMAIL,        Metadata::FMD_WEBSITE,
                                                Metadata::FMD_WEBSITE_MENU, Metadata::FMD_WIKIPEDIA};
  for (auto const type : kFields)
  {
    auto const value = obj.GetMetadata(type);
    if (value.empty())
      continue;
    // Wikipedia is stored as "lang:Article title" and is only useful in a mail as a link.
    place.m_fields.emplace_back(
        type, type == Metadata::FMD_WIKIPEDIA ? Metadata::ToWikiURL(std::string(value)) : std::string(value));
  }
}

Result Build(Place const & place, Strings const & strings)
{
  Result result;
  result.m_isMyPosition = place.m_isMyPosition;
  result.m_subjectBasis = !place.m_name.empty() ? place.m_name : place.m_address;

  // Staged rollout of the human-readable link, see https://github.com/organicmaps/organicmaps/pull/13100.
  //
  // Every app released before Ge0Parser::ParseClearCoordinates shipped claims omaps.app links
  // (Android app links, iOS universal links) and decodes them as ge0 base64 only, so a
  // "https://omaps.app/<lat>,<lon>" link silently opens such an app on the wrong place. Until that
  // parser is out in the wild we keep sharing the ge0 short link, which every version understands.
  //
  // TODO: when enough users run a version with ParseClearCoordinates (~a year after its release):
  //   1. call ge0::GenerateClearShowMapUrl(lat, lon, place.m_zoom, place.m_name) here;
  //   2. drop the coordinates line from the plain body below - the link then carries them itself;
  //   3. update the share_tests expectations.
  result.m_url = ge0::GenerateHttpShowMapUrl(place.m_ll.m_lat, place.m_ll.m_lon, place.m_zoom, place.m_name);

  std::string const geoUri = ge0::GenerateGeoUri(place.m_ll.m_lat, place.m_ll.m_lon, place.m_zoom, place.m_name);
  std::string const coords = measurement_utils::FormatLatLon(place.m_ll.m_lat, place.m_ll.m_lon, true /* withComma */);

  // The heading is the place name, or a "current position" phrase when sharing my position.
  std::string_view const heading =
      place.m_isMyPosition ? std::string_view{strings.m_myPosition} : std::string_view{place.m_name};

  // The place page has no separate name for an unnamed building (it shows the address as the title)
  // and no type for an unmatched map point (it shows the coordinates as the subtitle). Both lines
  // are printed below on their own, so drop the repetition instead of shipping it twice.
  std::string_view address = place.m_address;
  if (address == heading)
    address = {};
  std::string_view typeLabel = place.m_typeLabel;
  if (typeLabel == coords)
    typeLabel = {};

  // Plain body (messengers): heading, address, coordinates, link.
  for (std::string_view const line : {heading, address, std::string_view{coords}, std::string_view{result.m_url}})
  {
    if (line.empty())
      continue;
    if (!result.m_text.empty())
      result.m_text += '\n';
    result.m_text += line;
  }

  // HTML body (email): richer, with metadata and clickable links.
  std::string & html = result.m_html;
  if (!heading.empty())
  {
    html += "<b>";
    html += EscapeHtml(heading);
    html += "</b><br>\n";
  }
  if (!typeLabel.empty())
    AppendText(html, typeLabel);
  if (!address.empty())
    AppendText(html, address);
  for (auto const & [type, value] : place.m_fields)
    AppendHtmlField(html, type, value);

  // A bare map point with no address has nothing above the links - do not open the mail with an empty line.
  if (!html.empty())
    html += "<br>\n";
  AppendAnchor(html, result.m_url, strings.m_openInOmOrBrowser);
  AppendAnchor(html, geoUri, strings.m_openInMapsApp);
  html += "<br>\n";
  AppendText(html, coords);
  html += "<br>\n";
  AppendAnchor(html, "https://omaps.app/get", strings.m_getApp);

  return result;
}
}  // namespace share

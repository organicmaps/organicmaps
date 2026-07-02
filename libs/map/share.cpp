#include "map/share.hpp"

#include "ge0/url_generator.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"

#include <array>
#include <sstream>
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

std::string_view StripUrlScheme(std::string_view url)
{
  for (std::string_view const scheme : {"https://", "http://"})
    if (url.starts_with(scheme))
      return url.substr(scheme.size());
  return url;
}

std::string AddUrlScheme(std::string_view url)
{
  if (url.starts_with("https://") || url.starts_with("http://"))
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

void AppendAnchor(std::ostringstream & html, std::string_view href, std::string_view text)
{
  html << "<a href=\"" << EscapeHtml(href) << "\">" << EscapeHtml(text) << "</a><br>\n";
}

// Renders one allow-listed metadata field into the HTML body: phones/emails/sites become links,
// everything else is shown as text. Wiki values arrive already as URLs (ForEachMetadataReadable).
void AppendHtmlField(std::ostringstream & html, Metadata::EType type, std::string const & value)
{
  switch (type)
  {
  // OSM stores multiple values in one ";"-separated tag.
  case Metadata::FMD_PHONE_NUMBER:
    for (auto const phone : strings::TokenizeAndTrim(value, ";"))
      AppendAnchor(html, ToTelUri(phone), phone);
    break;
  case Metadata::FMD_EMAIL:
    for (auto const email : strings::TokenizeAndTrim(value, ";"))
      AppendAnchor(html, "mailto:" + std::string{email}, email);
    break;
  case Metadata::FMD_WEBSITE:
  case Metadata::FMD_WEBSITE_MENU:
    for (auto const site : strings::TokenizeAndTrim(value, ";"))
      AppendAnchor(html, AddUrlScheme(site), StripUrlScheme(site));
    break;
  case Metadata::FMD_WIKIPEDIA: AppendAnchor(html, value, "Wikipedia"); break;
  default: html << EscapeHtml(value) << "<br>\n"; break;
  }
}
}  // namespace

std::span<Metadata::EType const> SharedMetadataFields()
{
  static constexpr std::array kFields = {Metadata::FMD_OPEN_HOURS, Metadata::FMD_PHONE_NUMBER, Metadata::FMD_EMAIL,
                                         Metadata::FMD_WEBSITE,    Metadata::FMD_WEBSITE_MENU, Metadata::FMD_WIKIPEDIA};
  return kFields;
}

Result Build(Place const & place, Strings const & strings)
{
  Result result;
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
  std::string const & heading = place.m_isMyPosition ? strings.m_myPosition : place.m_name;

  // Plain body (messengers): heading, address, coordinates, link.
  {
    auto const appendLine = [&result](std::string const & line)
    {
      if (line.empty())
        return;
      if (!result.m_text.empty())
        result.m_text += '\n';
      result.m_text += line;
    };
    appendLine(heading);
    appendLine(place.m_address);
    appendLine(coords);
    appendLine(result.m_url);
  }

  // HTML body (email): richer, with metadata and clickable links.
  {
    std::ostringstream html;
    if (!heading.empty())
      html << "<b>" << EscapeHtml(heading) << "</b><br>\n";
    if (!place.m_isMyPosition && !place.m_typeLabel.empty())
      html << EscapeHtml(place.m_typeLabel) << "<br>\n";
    if (!place.m_address.empty())
      html << EscapeHtml(place.m_address) << "<br>\n";
    for (auto const & [type, value] : place.m_fields)
      AppendHtmlField(html, type, value);

    html << "<br>\n";
    AppendAnchor(html, result.m_url, strings.m_openInOmOrBrowser);
    AppendAnchor(html, geoUri, strings.m_openInMapsApp);
    html << "<br>\n" << EscapeHtml(coords) << "<br>\n<br>\n";
    AppendAnchor(html, "https://omaps.app/get", strings.m_getApp);
    result.m_html = html.str();
  }

  return result;
}
}  // namespace share

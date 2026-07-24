#include "map/share.hpp"

#include "ge0/url_generator.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"

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

// Splits an OSM-style multi-value field ("a;b;c") into trimmed, non-empty values.
std::vector<std::string> SplitValues(std::string const & value)
{
  std::vector<std::string> result;
  strings::Tokenize(value, ";", [&result](std::string_view token)
  {
    strings::Trim(token);
    if (!token.empty())
      result.emplace_back(token);
  });
  return result;
}

std::string_view StripUrlScheme(std::string_view url)
{
  for (std::string_view const scheme : {"https://", "http://"})
    if (url.starts_with(scheme))
      return url.substr(scheme.size());
  return url;
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
  case Metadata::FMD_PHONE_NUMBER:
    for (auto const & phone : SplitValues(value))
      AppendAnchor(html, "tel:" + phone, phone);
    break;
  case Metadata::FMD_EMAIL:
    for (auto const & email : SplitValues(value))
      AppendAnchor(html, "mailto:" + email, email);
    break;
  case Metadata::FMD_WEBSITE:
  case Metadata::FMD_WEBSITE_MENU:
    for (auto const & site : SplitValues(value))
      AppendAnchor(html, site, StripUrlScheme(site));
    break;
  case Metadata::FMD_WIKIPEDIA: AppendAnchor(html, value, "Wikipedia"); break;
  default: html << EscapeHtml(value) << "<br>\n"; break;
  }
}
}  // namespace

std::array<Metadata::EType, kSharedMetadataFieldsCount> const & SharedMetadataFields()
{
  static constexpr std::array kFields = {Metadata::FMD_OPEN_HOURS, Metadata::FMD_PHONE_NUMBER, Metadata::FMD_EMAIL,
                                         Metadata::FMD_WEBSITE,    Metadata::FMD_WEBSITE_MENU, Metadata::FMD_WIKIPEDIA};
  return kFields;
}

Result Build(Place const & place, Strings const & strings)
{
  Result result;
  result.m_subjectBasis = !place.m_name.empty() ? place.m_name : place.m_address;
  result.m_url = ge0::GenerateClearShowMapUrl(place.m_ll.m_lat, place.m_ll.m_lon, place.m_zoom, place.m_name);
  std::string const geoUri = ge0::GenerateGeoUri(place.m_ll.m_lat, place.m_ll.m_lon, place.m_zoom, place.m_name);
  std::string const coords = measurement_utils::FormatLatLon(place.m_ll.m_lat, place.m_ll.m_lon, true /* withComma */);

  // The heading is the place name, or a "current position" phrase when sharing my position.
  std::string const & heading = place.m_isMyPosition ? strings.m_myPosition : place.m_name;

  // Plain body (messengers): heading, address, link. The link already carries readable coordinates.
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

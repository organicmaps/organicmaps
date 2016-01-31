#include "editor/server_api.hpp"

#include "coding/url_encode.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/sstream.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace osm
{

ServerApi06::ServerApi06(OsmOAuth const & auth)
  : m_auth(auth)
{
}

uint64_t ServerApi06::CreateChangeSet(TKeyValueTags const & kvTags) const
{
  if (!m_auth.IsAuthorized())
    MYTHROW(NotAuthorized, ("Not authorized."));

  ostringstream stream;
  stream << "<osm>\n"
  "<changeset>\n";
  for (auto const & tag : kvTags)
    stream << "  <tag k=\"" << tag.first << "\" v=\"" << tag.second << "\"/>\n";
  stream << "</changeset>\n"
  "</osm>\n";

  OsmOAuth::Response const response = m_auth.Request("/changeset/create", "PUT", stream.str());
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(CreateChangeSetHasFailed, ("CreateChangeSet request has failed:", response));

  uint64_t id;
  if (!strings::to_uint64(response.second, id))
    MYTHROW(CantParseServerResponse, ("Can't parse changeset ID from server response."));
  return id;
}

uint64_t ServerApi06::CreateElement(editor::XMLFeature const & element) const
{
  OsmOAuth::Response const response = m_auth.Request("/" + element.GetTypeString() + "/create",
                                                     "PUT", element.ToOSMString());
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(CreateElementHasFailed, ("CreateElement request has failed:", response, "for", element));
  uint64_t id;
  if (!strings::to_uint64(response.second, id))
    MYTHROW(CantParseServerResponse, ("Can't parse created node ID from server response."));
  return id;
}

void ServerApi06::CreateElementAndSetAttributes(editor::XMLFeature & element) const
{
  uint64_t const id = CreateElement(element);
  element.SetAttribute("id", strings::to_string(id));
  element.SetAttribute("version", "1");
}

uint64_t ServerApi06::ModifyElement(editor::XMLFeature const & element) const
{
  string const id = element.GetAttribute("id");
  if (id.empty())
    MYTHROW(ModifiedElementHasNoIdAttribute, ("Please set id attribute for", element));

  OsmOAuth::Response const response = m_auth.Request("/" + element.GetTypeString() + "/" + id,
                                                     "PUT", element.ToOSMString());
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(ModifyElementHasFailed, ("ModifyElement request has failed:", response, "for", element));
  uint64_t version;
  if (!strings::to_uint64(response.second, version))
    MYTHROW(CantParseServerResponse, ("Can't parse element version from server response."));
  return version;
}

void ServerApi06::ModifyElementAndSetVersion(editor::XMLFeature & element) const
{
  uint64_t const version = ModifyElement(element);
  element.SetAttribute("version", strings::to_string(version));
}

bool ServerApi06::DeleteElement(editor::XMLFeature const & element) const
{
  string const id = element.GetAttribute("id");
  if (id.empty())
    MYTHROW(DeletedElementHasNoIdAttribute, ("Please set id attribute for", element));

  OsmOAuth::Response const response = m_auth.Request("/" + element.GetTypeString() + "/" + id,
                                                     "DELETE", element.ToOSMString());
  return (response.first == OsmOAuth::HTTP::OK || response.first == OsmOAuth::HTTP::Gone);
}

void ServerApi06::CloseChangeSet(uint64_t changesetId) const
{
  OsmOAuth::Response const response = m_auth.Request("/changeset/" + strings::to_string(changesetId) + "/close", "PUT");
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(ErrorClosingChangeSet, ("CloseChangeSet request has failed:", response));
}

bool ServerApi06::TestOSMUser(string const & userName)
{
  string const method = "/user/" + UrlEncode(userName);
  return m_auth.DirectRequest(method, false).first == OsmOAuth::HTTP::OK;
}

UserPreferences ServerApi06::GetUserPreferences() const
{
  OsmOAuth::Response const response = m_auth.Request("/user/details");
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(CantGetUserPreferences, (response));

  pugi::xml_document details;
  if (!details.load_string(response.second.c_str()))
    MYTHROW(CantParseUserPreferences, (response));

  pugi::xml_node const user = details.child("osm").child("user");
  if (!user || !user.attribute("id"))
    MYTHROW(CantParseUserPreferences, ("No <user> or 'id' attribute", response));

  UserPreferences pref;
  pref.m_id = user.attribute("id").as_ullong();
  pref.m_displayName = user.attribute("display_name").as_string();
  pref.m_accountCreated = my::StringToTimestamp(user.attribute("account_created").as_string());
  pref.m_imageUrl = user.child("img").attribute("href").as_string();
  pref.m_changesets = user.child("changesets").attribute("count").as_uint();
  return pref;
}

OsmOAuth::Response ServerApi06::GetXmlFeaturesInRect(m2::RectD const & latLonRect) const
{
  using strings::to_string_dac;

  // Digits After Comma.
  static constexpr double kDAC = 7;
  m2::PointD const lb = latLonRect.LeftBottom();
  m2::PointD const rt = latLonRect.RightTop();
  string const url = "/map?bbox=" + to_string_dac(lb.x, kDAC) + ',' + to_string_dac(lb.y, kDAC) + ',' +
      to_string_dac(rt.x, kDAC) + ',' + to_string_dac(rt.y, kDAC);

  return m_auth.DirectRequest(url);
}

OsmOAuth::Response ServerApi06::GetXmlFeaturesAtLatLon(double lat, double lon) const
{
  double const kInflateEpsilon = MercatorBounds::GetCellID2PointAbsEpsilon() / 2;
  m2::RectD rect(lon, lat, lon, lat);
  rect.Inflate(kInflateEpsilon, kInflateEpsilon);
  return GetXmlFeaturesInRect(rect);
}

OsmOAuth::Response ServerApi06::GetXmlFeaturesAtLatLon(ms::LatLon const & ll) const
{
  return GetXmlFeaturesAtLatLon(ll.lat, ll.lon);
}

} // namespace osm

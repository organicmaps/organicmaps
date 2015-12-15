#include "editor/server_api.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/sstream.hpp"

#include "3party/Alohalytics/src/http_client.h"

using alohalytics::HTTPClientPlatformWrapper;

namespace osm
{

namespace
{
void PrintRequest(HTTPClientPlatformWrapper const & r)
{
  LOG(LINFO, ("HTTP", r.http_method(), r.url_requested(), "has finished with code", r.error_code(),
      (r.was_redirected() ? ", was redirected to " + r.url_received() : ""),
      "Server replied:\n", r.server_response()));
}
} // namespace

ServerApi06::ServerApi06(string const & user, string const & password, string const & baseUrl)
  : m_user(user), m_password(password), m_baseOsmServerUrl(baseUrl)
{
}

bool ServerApi06::CreateChangeSet(TKeyValueTags const & kvTags, uint64_t & outChangeSetId) const
{
  ostringstream stream;
  stream << "<osm>\n"
  "<changeset>\n";
  for (auto const & tag : kvTags)
    stream << "  <tag k=\"" << tag.first << "\" v=\"" << tag.second << "\"/>\n";
  stream << "</changeset>\n"
  "</osm>\n";

  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/changeset/create");
  bool const success = request.set_user_and_password(m_user, m_password)
      .set_body_data(move(stream.str()), "", "PUT")
      .RunHTTPRequest();
  if (success && request.error_code() == 200)
  {
    if (strings::to_uint64(request.server_response(), outChangeSetId))
      return true;
    LOG(LWARNING, ("Can't parse changeset ID from server response."));
  }
  else
    LOG(LWARNING, ("CreateChangeSet request has failed."));

  PrintRequest(request);
  return false;
}

bool ServerApi06::CreateNode(string const & nodeXml, uint64_t & outCreatedNodeId) const
{
  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/node/create");
  bool const success = request.set_user_and_password(m_user, m_password)
      .set_body_data(move(nodeXml), "", "PUT")
      .RunHTTPRequest();
  if (success && request.error_code() == 200)
  {
    if (strings::to_uint64(request.server_response(), outCreatedNodeId))
      return true;
    LOG(LWARNING, ("Can't parse created node ID from server response."));
  }
  else
    LOG(LWARNING, ("CreateNode request has failed."));

  PrintRequest(request);
  return false;
}

bool ServerApi06::ModifyNode(string const & nodeXml, uint64_t nodeId) const
{
  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/node/" + strings::to_string(nodeId));
  bool const success = request.set_user_and_password(m_user, m_password)
      .set_body_data(move(nodeXml), "", "PUT")
      .RunHTTPRequest();
  if (success && request.error_code() == 200)
    return true;

  LOG(LWARNING, ("ModifyNode request has failed."));
  PrintRequest(request);
  return false;
}

ServerApi06::DeleteResult ServerApi06::DeleteNode(string const & nodeXml, uint64_t nodeId) const
{
  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/node/" + strings::to_string(nodeId));
  bool const success = request.set_user_and_password(m_user, m_password)
      .set_body_data(move(nodeXml), "", "DELETE")
      .RunHTTPRequest();
  if (success)
  {
    switch (request.error_code())
    {
    case 200: return DeleteResult::ESuccessfullyDeleted;
    case 412: return DeleteResult::ECanNotBeDeleted;
    }
  }

  LOG(LWARNING, ("DeleteNode request has failed."));
  PrintRequest(request);
  return DeleteResult::EFailed;
}

bool ServerApi06::CloseChangeSet(uint64_t changesetId) const
{
  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/changeset/" +
                                    strings::to_string(changesetId) + "/close");
  bool const success = request.set_user_and_password(m_user, m_password)
      .set_http_method("PUT")
      .RunHTTPRequest();
  if (success && request.error_code() == 200)
    return true;

  LOG(LWARNING, ("CloseChangeSet request has failed."));
  PrintRequest(request);
  return false;
}

bool ServerApi06::CheckUserAndPassword() const
{
  static constexpr char const * kAPIWritePermission = "allow_write_api";
  HTTPClientPlatformWrapper request(m_baseOsmServerUrl + "/api/0.6/permissions");
  bool const success = request.set_user_and_password(m_user, m_password).RunHTTPRequest();
  if (success && request.error_code() == 200 &&
      request.server_response().find(kAPIWritePermission) != string::npos)
    return true;

  LOG(LWARNING, ("OSM user and/or password are invalid."));
  PrintRequest(request);
  return false;
}

int ServerApi06::HttpCodeForUrl(string const & url)
{
  HTTPClientPlatformWrapper request(url);
  bool const success = request.RunHTTPRequest();
  int const httpCode = request.error_code();
  if (success)
    return httpCode;

  return -1;
}

string ServerApi06::GetXmlFeaturesInRect(m2::RectD const & latLonRect) const
{
  using strings::to_string_dac;

  // Digits After Comma.
  static constexpr double const kDAC = 7;
  m2::PointD const lb = latLonRect.LeftBottom();
  m2::PointD const rt = latLonRect.RightTop();
  string const url = m_baseOsmServerUrl + "/api/0.6/map?bbox=" + to_string_dac(lb.x, kDAC) + ',' + to_string_dac(lb.y, kDAC) + ',' +
      to_string_dac(rt.x, kDAC) + ',' + to_string_dac(rt.y, kDAC);
  HTTPClientPlatformWrapper request(url);
  bool const success = request.set_user_and_password(m_user, m_password).RunHTTPRequest();
  if (success && request.error_code() == 200)
    return request.server_response();

  LOG(LWARNING, ("GetXmlFeaturesInRect request has failed."));
  PrintRequest(request);
  return string();
}

string ServerApi06::GetXmlNodeByLatLon(double lat, double lon) const
{
  constexpr double const kInflateRadiusDegrees = 1.0e-6; //!< ~1 meter.
  m2::RectD rect(lon, lat, lon, lat);
  rect.Inflate(kInflateRadiusDegrees, kInflateRadiusDegrees);
  return GetXmlFeaturesInRect(rect);
}

} // namespace osm

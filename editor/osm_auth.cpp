#include "editor/osm_auth.hpp"

#include "platform/http_client.hpp"

#include "coding/url.hpp"

#include "base/string_utils.hpp"

#include "cppjansson/cppjansson.hpp"

#include "private.h"

namespace osm
{
using platform::HttpClient;
using std::string;

constexpr char const * kApiVersion = "/api/0.6";

namespace
{

string FindAuthenticityToken(string const & body)
{
  auto pos = body.find("name=\"authenticity_token\"");
  if (pos == string::npos)
    return {};
  string const kValue = "value=\"";
  auto start = body.find(kValue, pos);
  if (start == string::npos)
    return {};
  start += kValue.length();
  auto const end = body.find('"', start);
  return end == string::npos ? string() : body.substr(start, end - start);
}

// Parse URL in format "{OSM_OAUTH2_REDIRECT_URI}?code=XXXX". Extract code value
string FindOauthCode(string const & redirectUri)
{
  auto const url = url::Url::FromString(redirectUri);
  string const * oauth2code = url.GetParamValue("code");

  if (!oauth2code || oauth2code->empty())
    return {};

  return *oauth2code;
}

string FindAccessToken(string const & body)
{
  // Extract access_token from JSON in format {"access_token":"...", "token_type":"Bearer", "scope":"read_prefs"}
  const base::Json root(body.c_str());

  if (json_is_object(root.get()))
  {
    json_t * token_node = json_object_get(root.get(), "access_token");
    if (json_is_string(token_node))
      return json_string_value(token_node);
  }

  return {};
}

string BuildPostRequest(std::initializer_list<std::pair<string, string>> const & params)
{
  string result;
  for (auto it = params.begin(); it != params.end(); ++it)
  {
    if (it != params.begin())
      result += "&";
    result += it->first + "=" + url::UrlEncode(it->second);
  }
  return result;
}
}  // namespace

// static
bool OsmOAuth::IsValid(string const & ks)
{
  return !ks.empty();
}

OsmOAuth::OsmOAuth(string const & oauth2ClientId, string const & oauth2Secret, string const & oauth2Scope,
                   string const & oauth2RedirectUri, string baseUrl, string apiUrl)
  : m_oauth2params{oauth2ClientId, oauth2Secret, oauth2Scope, oauth2RedirectUri},
    m_baseUrl(std::move(baseUrl)), m_apiUrl(std::move(apiUrl))
{
}
// static
OsmOAuth OsmOAuth::ServerAuth()
{
#ifdef DEBUG
  return DevServerAuth();
#else
  return ProductionServerAuth();
#endif
}
// static
OsmOAuth OsmOAuth::ServerAuth(string const & oauthToken)
{
  OsmOAuth auth = ServerAuth();
  auth.SetAuthToken(oauthToken);
  return auth;
}
// static
OsmOAuth OsmOAuth::DevServerAuth()
{
  constexpr char const * kOsmDevServer = "https://master.apis.dev.openstreetmap.org";
  constexpr char const * kOsmDevClientId = "uB0deHjh_W86CRUHfvWlisCC1ZIHkdLoKxz1qkuIrrM";
  constexpr char const * kOsmDevClientSecret = "xQE7suO-jmzmels19k-m8FQ8gHnkdWuLLVqfW6FIj44";
  constexpr char const * kOsmDevScope = "read_prefs write_api write_notes";
  constexpr char const * kOsmDevRedirectUri = "om://oauth2/osm/callback";

  return {kOsmDevClientId, kOsmDevClientSecret, kOsmDevScope, kOsmDevRedirectUri, kOsmDevServer, kOsmDevServer};
}
// static
OsmOAuth OsmOAuth::ProductionServerAuth()
{
  constexpr char const * kOsmMainSiteURL = "https://www.openstreetmap.org";
  constexpr char const * kOsmApiURL = "https://api.openstreetmap.org";
  return {OSM_OAUTH2_CLIENT_ID, OSM_OAUTH2_CLIENT_SECRET, OSM_OAUTH2_SCOPE, OSM_OAUTH2_REDIRECT_URI, kOsmMainSiteURL, kOsmApiURL};
}

void OsmOAuth::SetAuthToken(string const & oauthToken) { m_oauth2token = oauthToken; }

string const & OsmOAuth::GetAuthToken() const { return m_oauth2token; }

bool OsmOAuth::IsAuthorized() const{ return IsValid(m_oauth2token); }

// Opens a login page and extract a cookie and a secret token.
OsmOAuth::SessionID OsmOAuth::FetchSessionId(string const & subUrl, string const & cookies) const
{
  string const url = m_baseUrl + subUrl + (cookies.empty() ? "?cookie_test=true" : "");
  HttpClient request(url);
  request.SetCookies(cookies);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("FetchSessionId Network error while connecting to", url));
  if (request.WasRedirected())
    MYTHROW(UnexpectedRedirect, ("FetchSessionId Unexpected redirected to", request.UrlReceived(), "from", url));
  if (request.ErrorCode() != HTTP::OK)
    MYTHROW(FetchSessionIdError, (DebugPrint(request)));

  SessionID sid = { request.CombinedCookies(), FindAuthenticityToken(request.ServerResponse()) };
  if (sid.m_cookies.empty() || sid.m_authenticityToken.empty())
    MYTHROW(FetchSessionIdError, ("Cookies and/or token are empty for request", DebugPrint(request)));
  return sid;
}

void OsmOAuth::LogoutUser(SessionID const & sid) const
{
  HttpClient request(m_baseUrl + "/logout");
  request.SetCookies(sid.m_cookies);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("LogoutUser Network error while connecting to", request.UrlRequested()));
  if (request.ErrorCode() != HTTP::OK)
    MYTHROW(LogoutUserError, (DebugPrint(request)));
}

bool OsmOAuth::LoginUserPassword(string const & login, string const & password, SessionID const & sid) const
{
  auto params = BuildPostRequest({
    {"username", login},
    {"password", password},
    {"referer", "/"},
    {"commit", "Login"},
    {"authenticity_token", sid.m_authenticityToken}
  });
  HttpClient request(m_baseUrl + "/login");
  request.SetBodyData(std::move(params), "application/x-www-form-urlencoded")
         .SetCookies(sid.m_cookies)
         .SetFollowRedirects(true);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("LoginUserPassword Network error while connecting to", request.UrlRequested()));

  // At the moment, automatic redirects handling is buggy on Androids < 4.4.
  // set_follow_redirects(false) works only for Android and iOS, while curl still automatically follow all redirects.
  if (request.ErrorCode() != HTTP::OK && request.ErrorCode() != HTTP::Found)
    MYTHROW(LoginUserPasswordServerError, (DebugPrint(request)));

  // Not redirected page is a 100% signal that login and/or password are invalid.
  if (!request.WasRedirected())
    return false;

  // Check if we were redirected to some 3rd party site.
  if (request.UrlReceived().find(m_baseUrl) != 0)
    MYTHROW(UnexpectedRedirect, (DebugPrint(request)));

  // m_baseUrl + "/login" means login and/or password are invalid.
  return request.ServerResponse().find("/login") == string::npos;
}

bool OsmOAuth::LoginSocial(string const & callbackPart, string const & socialToken, SessionID const & sid) const
{
  string const url = m_baseUrl + callbackPart + socialToken;
  HttpClient request(url);
  request.SetCookies(sid.m_cookies)
         .SetFollowRedirects(true);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("LoginSocial Network error while connecting to", request.UrlRequested()));
  if (request.ErrorCode() != HTTP::OK && request.ErrorCode() != HTTP::Found)
    MYTHROW(LoginSocialServerError, (DebugPrint(request)));

  // Not redirected page is a 100% signal that social login has failed.
  if (!request.WasRedirected())
    return false;

  // Check if we were redirected to some 3rd party site.
  if (request.UrlReceived().find(m_baseUrl) != 0)
    MYTHROW(UnexpectedRedirect, (DebugPrint(request)));

  // m_baseUrl + "/login" means login and/or password are invalid.
  return request.ServerResponse().find("/login") == string::npos;
}

// Fakes a buttons press to automatically accept requested permissions.
string OsmOAuth::SendAuthRequest(string const & requestTokenKey, SessionID const & lastSid) const
{
  auto params = BuildPostRequest({
    {"authenticity_token", requestTokenKey},
    {"client_id", m_oauth2params.m_clientId},
    {"redirect_uri", m_oauth2params.m_redirectUri},
    {"scope", m_oauth2params.m_scope},
    {"response_type", "code"}
  });
  HttpClient request(m_baseUrl + "/oauth2/authorize");
  request.SetBodyData(std::move(params), "application/x-www-form-urlencoded")
         .SetCookies(lastSid.m_cookies)
         //.SetRawHeader("Origin", m_baseUrl)
         .SetFollowRedirects(false);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("SendAuthRequest Network error while connecting to", request.UrlRequested()));
  if (!request.WasRedirected())
    MYTHROW(UnexpectedRedirect, ("Expected redirect for URL", request.UrlRequested()));

  // Recieved URL in format "{OSM_OAUTH2_REDIRECT_URI}?code=XXXX". Extract code value
  string const oauthCode = FindOauthCode(request.UrlReceived());
  if (oauthCode.empty())
    MYTHROW(OsmOAuth::NetworkError, ("SendAuthRequest Redirect url has no 'code' parameter", request.UrlReceived()));
  return oauthCode;
}

string OsmOAuth::FetchRequestToken(SessionID const & sid) const
{
  HttpClient request(BuildOAuth2Url());
  request.SetCookies(sid.m_cookies)
         .SetFollowRedirects(false);

  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("FetchRequestToken Network error while connecting to", request.UrlRequested()));

  if (request.WasRedirected())
  {
    if (request.UrlReceived().find(m_oauth2params.m_redirectUri) != 0)
      MYTHROW(OsmOAuth::NetworkError, ("FetchRequestToken Redirect url han unexpected prefix", request.UrlReceived()));

    // User already accepted OAuth2 request.
    // Recieved URL in format "{OSM_OAUTH2_REDIRECT_URI}?code=XXXX". Extract code value
    string const oauthCode = FindOauthCode(request.UrlReceived());
    if (oauthCode.empty())
      MYTHROW(OsmOAuth::NetworkError, ("FetchRequestToken Redirect url has no 'code' parameter", request.UrlReceived()));
    return oauthCode;
  }
  else
  {
    if (request.ErrorCode() != HTTP::OK)
      MYTHROW(FetchRequestTokenServerError, (DebugPrint(request)));

    // Throws std::runtime_error.
    string const authenticityToken = FindAuthenticityToken(request.ServerResponse());

    // Accept OAuth2 request from server
    return SendAuthRequest(authenticityToken, sid);
  }
}

string OsmOAuth::BuildOAuth2Url() const
{
   auto requestTokenUrl = m_baseUrl + "/oauth2/authorize";
   auto const requestTokenQuery = BuildPostRequest(
   {
       {"client_id", m_oauth2params.m_clientId},
       {"redirect_uri", m_oauth2params.m_redirectUri},
       {"scope", m_oauth2params.m_scope},
       {"response_type", "code"}
   });
   return requestTokenUrl.append("?").append(requestTokenQuery);
}

string OsmOAuth::FinishAuthorization(string const & oauth2code) const
{
  auto params = BuildPostRequest({
      {"grant_type", "authorization_code"},
      {"code", oauth2code},
      {"client_id", m_oauth2params.m_clientId},
      {"client_secret", m_oauth2params.m_clientSecret},
      {"redirect_uri", m_oauth2params.m_redirectUri},
      {"scope", m_oauth2params.m_scope},
  });

  HttpClient request(m_baseUrl + "/oauth2/token");
  request.SetBodyData(std::move(params), "application/x-www-form-urlencoded")
      .SetFollowRedirects(true);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("FinishAuthorization Network error while connecting to", request.UrlRequested()));
  if (request.ErrorCode() != HTTP::OK)
    MYTHROW(FinishAuthorizationServerError, (DebugPrint(request)));
  if (request.WasRedirected())
    MYTHROW(UnexpectedRedirect, ("Redirected to", request.UrlReceived(), "from", request.UrlRequested()));

  // Parse response JSON
  return FindAccessToken(request.ServerResponse());
}

// Given a web session id, fetches an OAuth access token.
string OsmOAuth::FetchAccessToken(SessionID const & sid) const
{
  // Faking a button press for access rights.
  string const oauth2code = FetchRequestToken(sid);
  LogoutUser(sid);

  // Got code, exchange it for the access token.
  return FinishAuthorization(oauth2code);
}

bool OsmOAuth::AuthorizePassword(string const & login, string const & password)
{
  SessionID const sid = FetchSessionId();
  if (!LoginUserPassword(login, password, sid))
    return false;
  m_oauth2token = FetchAccessToken(sid);
  return true;
}

/// @todo OSM API to reset password has changed and should be updated
/*
bool OsmOAuth::ResetPassword(string const & email) const
{
  string const kForgotPasswordUrlPart = "/user/forgot-password";

  SessionID const sid = FetchSessionId(kForgotPasswordUrlPart);
  auto params = BuildPostRequest({
    {"email", email},
    {"authenticity_token", sid.m_authenticityToken},
    {"commit", "Reset password"},
  });
  HttpClient request(m_baseUrl + kForgotPasswordUrlPart);
  request.SetBodyData(std::move(params), "application/x-www-form-urlencoded");
  request.SetCookies(sid.m_cookies);
  request.SetHandleRedirects(false);

  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("ResetPassword Network error while connecting to", request.UrlRequested()));
  if (request.ErrorCode() != HTTP::OK)
    MYTHROW(ResetPasswordServerError, (DebugPrint(request)));

  if (request.WasRedirected() && request.UrlReceived().find(m_baseUrl) != string::npos)
    return true;
  return false;
}
*/

OsmOAuth::Response OsmOAuth::Request(string const & method, string const & httpMethod, string const & body) const
{
  if (!IsValid(m_oauth2token))
    MYTHROW(InvalidKeySecret, ("Auth token is empty."));

  string url = m_apiUrl + kApiVersion + method;

  HttpClient request(url);
  request.SetRawHeader("Authorization", "Bearer " + m_oauth2token);

  if (httpMethod != "GET")
    request.SetBodyData(body, "application/xml", httpMethod);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("Request Network error while connecting to", url));
  if (request.WasRedirected())
    MYTHROW(UnexpectedRedirect, ("Redirected to", request.UrlReceived(), "from", url));

  return {request.ErrorCode(), request.ServerResponse()};
}

OsmOAuth::Response OsmOAuth::DirectRequest(string const & method, bool api) const
{
  string const url = api ? m_apiUrl + kApiVersion + method : m_baseUrl + method;
  HttpClient request(url);
  if (!request.RunHttpRequest())
    MYTHROW(NetworkError, ("DirectRequest Network error while connecting to", url));
  if (request.WasRedirected())
    MYTHROW(UnexpectedRedirect, ("Redirected to", request.UrlReceived(), "from", url));

  return {request.ErrorCode(), request.ServerResponse()};
}

string DebugPrint(OsmOAuth::Response const & code)
{
  string r;
  switch (code.first)
  {
  case OsmOAuth::HTTP::OK: r = "OK"; break;
  case OsmOAuth::HTTP::BadXML: r = "BadXML"; break;
  case OsmOAuth::HTTP::BadAuth: r = "BadAuth"; break;
  case OsmOAuth::HTTP::Redacted: r = "Redacted"; break;
  case OsmOAuth::HTTP::NotFound: r = "NotFound"; break;
  case OsmOAuth::HTTP::WrongMethod: r = "WrongMethod"; break;
  case OsmOAuth::HTTP::Conflict: r = "Conflict"; break;
  case OsmOAuth::HTTP::Gone: r = "Gone"; break;
  case OsmOAuth::HTTP::PreconditionFailed: r = "PreconditionFailed"; break;
  case OsmOAuth::HTTP::URITooLong: r = "URITooLong"; break;
  case OsmOAuth::HTTP::TooMuchData: r = "TooMuchData"; break;
  default:
    // No data from server in case of NetworkError.
    if (code.first < 0)
      return "NetworkError " + strings::to_string(code.first);
    r = "HTTP " + strings::to_string(code.first);
  }
  return r + ": " + code.second;
}

}  // namespace osm

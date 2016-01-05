#include "editor/osm_auth.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "coding/url_encode.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "3party/liboauthcpp/include/liboauthcpp/liboauthcpp.h"
#include "3party/Alohalytics/src/http_client.h"

using alohalytics::HTTPClientPlatformWrapper;

namespace osm
{
constexpr char const * kApiVersion = "/api/0.6";

namespace
{
string findAuthenticityToken(string const & body)
{
  auto pos = body.find("name=\"authenticity_token\"");
  if (pos == string::npos)
    return string();
  string const kValue = "value=\"";
  auto start = body.find(kValue, pos);
  if (start == string::npos)
    return string();
  start += kValue.length();
  auto const end = body.find("\"", start);
  return end == string::npos ? string() : body.substr(start, end - start);
}

string buildPostRequest(map<string, string> const & params)
{
  string result;
  for (auto it = params.begin(); it != params.end(); ++it)
  {
    if (it != params.begin())
      result += "&";
    result += it->first + "=" + UrlEncode(it->second);
  }
  return result;
}

// Trying to determine whether it's a login page.
bool isLoggedIn(string const & contents)
{
  return contents.find("<form id=\"login_form\"") == string::npos;
}
}  // namespace

// Opens a login page and extract a cookie and a secret token.
OsmOAuth::AuthResult OsmOAuth::FetchSessionId(OsmOAuth::SessionID & sid) const
{
  HTTPClientPlatformWrapper request(m_baseUrl + "/login?cookie_test=true");
  if (!request.RunHTTPRequest())
    return AuthResult::NetworkError;
  if (request.error_code() != 200)
    return AuthResult::ServerError;
  sid.m_cookies = request.combined_cookies();
  sid.m_token = findAuthenticityToken(request.server_response());
  return !sid.m_cookies.empty() && !sid.m_token.empty() ? AuthResult::OK : AuthResult::FailCookie;
}

// Log a user out.
OsmOAuth::AuthResult OsmOAuth::LogoutUser(SessionID const & sid) const
{
  HTTPClientPlatformWrapper request(m_baseUrl + "/logout");
  request.set_cookies(sid.m_cookies);
  if (!request.RunHTTPRequest())
    return AuthResult::NetworkError;
  if (request.error_code() != 200)
    return AuthResult::ServerError;
  return AuthResult::OK;
}

// Signs a user id using login and password.
OsmOAuth::AuthResult OsmOAuth::LoginUserPassword(string const & login, string const & password, SessionID const & sid) const
{
  map<string, string> params;
  params["username"] = login;
  params["password"] = password;
  params["referer"] = "/";
  params["commit"] = "Login";
  params["authenticity_token"] = sid.m_token;
  HTTPClientPlatformWrapper request(m_baseUrl + "/login");
  request.set_body_data(buildPostRequest(params), "application/x-www-form-urlencoded");
  request.set_cookies(sid.m_cookies);
  if (!request.RunHTTPRequest())
    return AuthResult::NetworkError;
  if (request.error_code() != 200)
    return AuthResult::ServerError;
  if (!request.was_redirected())
    return AuthResult::FailLogin;
  // Since we don't know whether the request was redirected or not, we need to check page contents.
  return isLoggedIn(request.server_response()) ? AuthResult::OK : AuthResult::FailLogin;
}

// Signs a user in using a facebook token.
OsmOAuth::AuthResult OsmOAuth::LoginFacebook(string const & facebookToken, SessionID const & sid) const
{
  string const url = m_baseUrl + "/auth/facebook_access_token/callback?access_token=" + facebookToken;
  HTTPClientPlatformWrapper request(url);
  request.set_cookies(sid.m_cookies);
  if (!request.RunHTTPRequest())
    return AuthResult::NetworkError;
  if (request.error_code() != 200)
    return AuthResult::ServerError;
  if (!request.was_redirected())
    return AuthResult::FailLogin;
  return isLoggedIn(request.server_response()) ? AuthResult::OK : AuthResult::FailLogin;
}

// Fakes a buttons press, so a user accepts requested permissions.
string OsmOAuth::SendAuthRequest(string const & requestTokenKey, SessionID const & sid) const
{
  map<string, string> params;
  params["oauth_token"] = requestTokenKey;
  params["oauth_callback"] = "";
  params["authenticity_token"] = sid.m_token;
  params["allow_read_prefs"] = "yes";
  params["allow_write_api"] = "yes";
  params["commit"] = "Save changes";
  HTTPClientPlatformWrapper request(m_baseUrl + "/oauth/authorize");
  request.set_body_data(buildPostRequest(params), "application/x-www-form-urlencoded");
  request.set_cookies(sid.m_cookies);

  if (!request.RunHTTPRequest())
    return string();

  string const callbackURL = request.url_received();
  string const vKey = "oauth_verifier=";
  auto const pos = callbackURL.find(vKey);
  if (pos == string::npos)
    return string();
  auto const end = callbackURL.find("&", pos);
  return callbackURL.substr(pos + vKey.length(), end == string::npos ? end : end - pos - vKey.length()+ 1);
}

// Given a web session id, fetches an OAuth access token.
OsmOAuth::AuthResult OsmOAuth::FetchAccessToken(SessionID const & sid, ClientToken & token) const
{
  // Aquire a request token.
  OAuth::Consumer consumer(m_consumerKey, m_consumerSecret);
  OAuth::Client oauth(&consumer);
  string const requestTokenUrl = m_baseUrl + "/oauth/request_token";
  string const requestTokenQuery = oauth.getURLQueryString(OAuth::Http::Get, requestTokenUrl + "?oauth_callback=oob");
  HTTPClientPlatformWrapper request(requestTokenUrl + "?" + requestTokenQuery);
  if (!(request.RunHTTPRequest() && request.error_code() == 200 && !request.was_redirected()))
    return AuthResult::NoOAuth;
  OAuth::Token requestToken = OAuth::Token::extract(request.server_response());

  // Faking a button press for access rights.
  string const pin = SendAuthRequest(requestToken.key(), sid);
  if (pin.empty())
    return AuthResult::FailAuth;
  requestToken.setPin(pin);

  // Got pin, exchange it for the access token.
  oauth = OAuth::Client(&consumer, &requestToken);
  string const accessTokenUrl = m_baseUrl + "/oauth/access_token";
  string const queryString = oauth.getURLQueryString(OAuth::Http::Get, accessTokenUrl, "", true);
  HTTPClientPlatformWrapper request2(accessTokenUrl + "?" + queryString);
  if (!(request2.RunHTTPRequest() && request2.error_code() == 200 && !request2.was_redirected()))
    return AuthResult::NoAccess;
  OAuth::KeyValuePairs responseData = OAuth::ParseKeyValuePairs(request2.server_response());
  OAuth::Token accessToken = OAuth::Token::extract(responseData);

  token.m_key = accessToken.key();
  token.m_secret = accessToken.secret();

  LogoutUser(sid);

  return AuthResult::OK;
}

OsmOAuth::AuthResult OsmOAuth::AuthorizePassword(string const & login, string const & password, ClientToken & token) const
{
  SessionID sid;
  AuthResult result = FetchSessionId(sid);
  if (result != AuthResult::OK)
    return result;

  result = LoginUserPassword(login, password, sid);
  if (result != AuthResult::OK)
    return result;

  return FetchAccessToken(sid, token);
}

OsmOAuth::AuthResult OsmOAuth::AuthorizeFacebook(string const & facebookToken, ClientToken & token) const
{
  SessionID sid;
  AuthResult result = FetchSessionId(sid);
  if (result != AuthResult::OK)
    return result;

  result = LoginFacebook(facebookToken, sid);
  if (result != AuthResult::OK)
    return result;

  return FetchAccessToken(sid, token);
}

string OsmOAuth::Request(ClientToken const & token, string const & method, string const & httpMethod, string const & body) const
{
  OAuth::Consumer const consumer(m_consumerKey, m_consumerSecret);
  OAuth::Token const oatoken(token.m_key, token.m_secret);
  OAuth::Client oauth(&consumer, &oatoken);

  OAuth::Http::RequestType reqType;
  if (httpMethod == "GET")
    reqType = OAuth::Http::Get;
  else if (httpMethod == "POST")
    reqType = OAuth::Http::Post;
  else if (httpMethod == "PUT")
    reqType = OAuth::Http::Put;
  else
  {
    ASSERT(false, ("Unsupported OSM API request method", httpMethod));
    return string();
  }

  string const url = m_apiUrl + kApiVersion + method;
  string const query = oauth.getURLQueryString(reqType, url, body);

  HTTPClientPlatformWrapper request(url + "?" + query);
  if (httpMethod != "GET")
    request.set_body_data(body, "application/xml", httpMethod);
  if (request.RunHTTPRequest() && request.error_code() == 200 && !request.was_redirected())
    return request.server_response();

  return string();
}

}  // namespace osm

#pragma once

#include "std/string.hpp"
#include "std/utility.hpp"

namespace osm
{

using TKeySecret = pair<string /*key*/, string /*secret*/>;

class OsmOAuth
{
public:
  /// A result of authentication. OK if everything is good.
  enum class AuthResult
  {
    OK,
    FailCookie,
    FailLogin,
    NoOAuth,
    FailAuth,
    NoAccess,
    NetworkError,
    ServerError
  };

  /// A result of a request. Has readable values for all OSM API return codes.
  enum class ResponseCode
  {
    NetworkError = -1,
    OK = 200,
    BadXML = 400,
    BadAuth = 401,
    Redacted = 403,
    NotFound = 404,
    WrongMethod = 405,
    Conflict = 409,
    Gone = 410,
    RefError = 412,
    URITooLong = 414,
    TooMuchData = 509
  };

  /// A pair of <error code, response contents>.
  using Response = std::pair<ResponseCode, string>;
  /// A pair of <url, key-secret>.
  using TUrlKeySecret = std::pair<string, TKeySecret>;

  /// The constructor. Simply stores a lot of strings in fields.
  /// @param[apiUrl] The OSM API URL defaults to baseUrl, or kDefaultApiURL if not specified.
  OsmOAuth(string const & consumerKey, string const & consumerSecret, string const & baseUrl, string const & apiUrl);

  /// Ilya Zverev's test server.
  static OsmOAuth IZServerAuth();
  /// master.apis.dev.openstreetmap.org
  static OsmOAuth DevServerAuth();
  /// api.openstreetmap.org
  static OsmOAuth ProductionServerAuth();
  /// Should be used everywhere in production code.
  static OsmOAuth ServerAuth();

  /// @name Stateless methods.
  //@{
  AuthResult AuthorizePassword(string const & login, string const & password, TKeySecret & outKeySecret) const;
  AuthResult AuthorizeFacebook(string const & facebookToken, TKeySecret & outKeySecret) const;
  AuthResult AuthorizeGoogle(string const & googleToken, TKeySecret & outKeySecret) const;
  /// @param[method] The API method, must start with a forward slash.
  Response Request(TKeySecret const & keySecret, string const & method, string const & httpMethod = "GET", string const & body = "") const;
  //@}

  /// @name Methods for using a token stored in this class. Obviously not thread-safe.
  //@{
  OsmOAuth & SetToken(TKeySecret const & keySecret);
  TKeySecret const & GetToken() const;
  bool IsAuthorized() const;
  AuthResult AuthorizePassword(string const & login, string const & password);
  AuthResult AuthorizeFacebook(string const & facebookToken);
  AuthResult AuthorizeGoogle(string const & googleToken);
  /// @param[method] The API method, must start with a forward slash.
  Response Request(string const & method, string const & httpMethod = "GET", string const & body = "") const;
  //@}

  /// @name Methods for WebView-based authentication.
  //@{
  TUrlKeySecret GetFacebookOAuthURL() const;
  TUrlKeySecret GetGoogleOAuthURL() const;
  AuthResult FinishAuthorization(TKeySecret const & requestToken, string const & verifier, TKeySecret & outKeySecret) const;
  AuthResult FinishAuthorization(TKeySecret const & requestToken, string const & verifier);
  //@}

  /// Tokenless GET request, for convenience.
  /// @param[api] If false, request is made to m_baseUrl.
  Response DirectRequest(string const & method, bool api = true) const;

private:
  struct SessionID
  {
    string m_cookies;
    string m_token;
  };

  /// Key and secret for application.
  TKeySecret m_consumerKeySecret;
  string m_baseUrl;
  string m_apiUrl;
  /// Key and secret to sign every OAuth request.
  TKeySecret m_tokenKeySecret;

  AuthResult FetchSessionId(SessionID & sid) const;
  AuthResult LogoutUser(SessionID const & sid) const;
  AuthResult LoginUserPassword(string const & login, string const & password, SessionID const & sid) const;
  AuthResult LoginSocial(string const & callbackPart, string const & socialToken, SessionID const & sid) const;
  string SendAuthRequest(string const & requestTokenKey, SessionID const & sid) const;
  TKeySecret FetchRequestToken() const;
  AuthResult FetchAccessToken(SessionID const & sid, TKeySecret & outKeySecret) const;
  AuthResult FetchAccessToken(SessionID const & sid);
};

string DebugPrint(OsmOAuth::AuthResult const res);
string DebugPrint(OsmOAuth::ResponseCode const code);

}  // namespace osm

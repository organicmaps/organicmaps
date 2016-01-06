#pragma once

#include "std/string.hpp"

namespace osm
{

/// A structure that holds request token pair.
struct ClientToken
{
  string m_key;
  string m_secret;
  inline bool IsValid() const { return !m_key.empty() && !m_secret.empty(); }
};

constexpr char const * kDefaultBaseURL = "https://www.openstreetmap.org";
constexpr char const * kDefaultApiURL = "https://api.openstreetmap.org";

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
    ServerError = -1,
    OK = 200,
    BadXML = 400,
    Redacted = 403,
    NotFound = 404,
    WrongMethod = 405,
    Conflict = 409,
    Gone = 410,
    RefError = 412,
    URITooLong = 414,
    TooMuchData = 509
  };

  /// A pair of <error code, response contents>
  using Response = std::pair<ResponseCode, string>;

  /// The constructor. Simply stores a lot of strings in fields.
  /// @param[apiUrl] The OSM API URL defaults to baseUrl, or kDefaultApiURL if not specified.
  OsmOAuth(string const & consumerKey, string const & consumerSecret,
           string const & baseUrl = kDefaultBaseURL,
           string const & apiUrl = kDefaultApiURL);

  /// @name Stateless methods.
  //@{
  AuthResult AuthorizePassword(string const & login, string const & password, ClientToken & token) const;
  AuthResult AuthorizeFacebook(string const & facebookToken, ClientToken & token) const;
  /// @param[method] The API method, must start with a forward slash.
  Response Request(ClientToken const & token, string const & method, string const & httpMethod = "GET", string const & body = "") const;
  //@}

  /// @name Methods for using a token stored in this class. Obviously not thread-safe.
  //@{
  void SetToken(ClientToken const & token);
  ClientToken const & GetToken() const { return m_token; }
  bool IsAuthorized() const { return m_token.IsValid(); }
  AuthResult AuthorizePassword(string const & login, string const & password);
  AuthResult AuthorizeFacebook(string const & facebookToken);
  /// @param[method] The API method, must start with a forward slash.
  Response Request(string const & method, string const & httpMethod = "GET", string const & body = "") const;
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

  string m_consumerKey;
  string m_consumerSecret;
  string m_baseUrl;
  string m_apiUrl;
  ClientToken m_token;

  AuthResult FetchSessionId(SessionID & sid) const;
  AuthResult LogoutUser(SessionID const & sid) const;
  AuthResult LoginUserPassword(string const & login, string const & password, SessionID const & sid) const;
  AuthResult LoginFacebook(string const & facebookToken, SessionID const & sid) const;
  string SendAuthRequest(string const & requestTokenKey, SessionID const & sid) const;
  AuthResult FetchAccessToken(SessionID const & sid, ClientToken & token) const;
  AuthResult FetchAccessToken(SessionID const & sid);
};

string DebugPrint(OsmOAuth::AuthResult const res);
string DebugPrint(OsmOAuth::ResponseCode const code);

}  // namespace osm

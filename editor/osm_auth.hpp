#pragma once

#include "base/exception.hpp"

#include <string>
#include <utility>

namespace osm
{
using KeySecret = std::pair<std::string /*key*/, std::string /*secret*/>;
using RequestToken = KeySecret;

/// All methods that interact with the OSM server are blocking and not asynchronous.
class OsmOAuth
{
public:
  /// Do not use enum class here for easier matching with error_code().
  enum HTTP : int
  {
    OK = 200,
    Found = 302,
    BadXML = 400,
    BadAuth = 401,
    Redacted = 403,
    NotFound = 404,
    WrongMethod = 405,
    Conflict = 409,
    Gone = 410,
    // Most often it means bad reference to another object.
    PreconditionFailed = 412,
    URITooLong = 414,
    TooMuchData = 509
  };

  /// A pair of <http error code, response contents>.
  using Response = std::pair<int, std::string>;
  /// A pair of <url, key-secret>.
  using UrlRequestToken = std::pair<std::string, RequestToken>;

  DECLARE_EXCEPTION(OsmOAuthException, RootException);
  DECLARE_EXCEPTION(NetworkError, OsmOAuthException);
  DECLARE_EXCEPTION(UnexpectedRedirect, OsmOAuthException);
  DECLARE_EXCEPTION(UnsupportedApiRequestMethod, OsmOAuthException);
  DECLARE_EXCEPTION(InvalidKeySecret, OsmOAuthException);
  DECLARE_EXCEPTION(FetchSessionIdError, OsmOAuthException);
  DECLARE_EXCEPTION(LogoutUserError, OsmOAuthException);
  DECLARE_EXCEPTION(LoginUserPasswordServerError, OsmOAuthException);
  DECLARE_EXCEPTION(LoginUserPasswordFailed, OsmOAuthException);
  DECLARE_EXCEPTION(LoginSocialServerError, OsmOAuthException);
  DECLARE_EXCEPTION(LoginSocialFailed, OsmOAuthException);
  DECLARE_EXCEPTION(SendAuthRequestError, OsmOAuthException);
  DECLARE_EXCEPTION(FetchRequestTokenServerError, OsmOAuthException);
  DECLARE_EXCEPTION(FinishAuthorizationServerError, OsmOAuthException);
  DECLARE_EXCEPTION(ResetPasswordServerError, OsmOAuthException);

  static bool IsValid(KeySecret const & ks) noexcept;
  static bool IsValid(UrlRequestToken const & urt) noexcept;

  /// The constructor. Simply stores a lot of strings in fields.
  OsmOAuth(std::string const & consumerKey, std::string const & consumerSecret,
           std::string const & baseUrl, std::string const & apiUrl) noexcept;

  /// Should be used everywhere in production code instead of servers below.
  static OsmOAuth ServerAuth() noexcept;
  static OsmOAuth ServerAuth(KeySecret const & userKeySecret) noexcept;

  /// Ilya Zverev's test server.
  static OsmOAuth IZServerAuth() noexcept;
  /// master.apis.dev.openstreetmap.org
  static OsmOAuth DevServerAuth() noexcept;
  /// api.openstreetmap.org
  static OsmOAuth ProductionServerAuth() noexcept;

  void SetKeySecret(KeySecret const & keySecret) noexcept;
  KeySecret const & GetKeySecret() const noexcept;
  bool IsAuthorized() const noexcept;

  /// @returns false if login and/or password are invalid.
  bool AuthorizePassword(std::string const & login, std::string const & password);
  /// @returns false if Facebook credentials are invalid.
  bool AuthorizeFacebook(std::string const & facebookToken);
  /// @returns false if Google credentials are invalid.
  bool AuthorizeGoogle(std::string const & googleToken);
  /// @returns false if email has not been registered on a server.
  bool ResetPassword(std::string const & email) const;

  /// Throws in case of network errors.
  /// @param[method] The API method, must start with a forward slash.
  Response Request(std::string const & method, std::string const & httpMethod = "GET",
                   std::string const & body = "") const;
  /// Tokenless GET request, for convenience.
  /// @param[api] If false, request is made to m_baseUrl.
  Response DirectRequest(std::string const & method, bool api = true) const;

  /// @name Methods for WebView-based authentication.
  //@{
  UrlRequestToken GetFacebookOAuthURL() const;
  UrlRequestToken GetGoogleOAuthURL() const;
  KeySecret FinishAuthorization(RequestToken const & requestToken,
                                std::string const & verifier) const;
  // AuthResult FinishAuthorization(KeySecret const & requestToken, string const & verifier);
  std::string GetRegistrationURL() const noexcept { return m_baseUrl + "/user/new"; }
  std::string GetResetPasswordURL() const noexcept { return m_baseUrl + "/user/forgot-password"; }
  //@}

private:
  struct SessionID
  {
    std::string m_cookies;
    std::string m_token;
  };

  /// Key and secret for application.
  KeySecret const m_consumerKeySecret;
  std::string const m_baseUrl;
  std::string const m_apiUrl;
  /// Key and secret to sign every OAuth request.
  KeySecret m_tokenKeySecret;

  SessionID FetchSessionId(std::string const & subUrl = "/login",
                           std::string const & cookies = "") const;
  /// Log a user out.
  void LogoutUser(SessionID const & sid) const;
  /// Signs a user id using login and password.
  /// @returns false if login or password are invalid.
  bool LoginUserPassword(std::string const & login, std::string const & password,
                         SessionID const & sid) const;
  /// Signs a user in using Facebook token.
  /// @returns false if the social token is invalid.
  bool LoginSocial(std::string const & callbackPart, std::string const & socialToken,
                   SessionID const & sid) const;
  /// @returns non-empty string with oauth_verifier value.
  std::string SendAuthRequest(std::string const & requestTokenKey, SessionID const & lastSid) const;
  /// @returns valid key and secret or throws otherwise.
  RequestToken FetchRequestToken() const;
  KeySecret FetchAccessToken(SessionID const & sid) const;
  //AuthResult FetchAccessToken(SessionID const & sid);
};

std::string DebugPrint(OsmOAuth::Response const & code);

}  // namespace osm

#pragma once

#include "std/string.hpp"

namespace osm
{

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
  enum AuthResult
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

  OsmOAuth(string const & consumerKey, string const & consumerSecret,
           string const & baseUrl = kDefaultBaseURL,
           string const & apiUrl = kDefaultApiURL):
      m_consumerKey(consumerKey),
      m_consumerSecret(consumerSecret),
      m_baseUrl(baseUrl),
      m_apiUrl(apiUrl)
  {
  }

  AuthResult AuthorizePassword(string const & login, string const & password, ClientToken & token) const;
  AuthResult AuthorizeFacebook(string const & facebookToken, ClientToken & token) const;
  string Request(ClientToken const & token, string const & method, string const & httpMethod = "GET", string const & body = "") const;

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

  AuthResult FetchSessionId(SessionID & sid) const;
  AuthResult LogoutUser(SessionID const & sid) const;
  AuthResult LoginUserPassword(string const & login, string const & password, SessionID const & sid) const;
  AuthResult LoginFacebook(string const & facebookToken, SessionID const & sid) const;
  string SendAuthRequest(string const & requestTokenKey, SessionID const & sid) const;
  AuthResult FetchAccessToken(SessionID const & sid, ClientToken & token) const;
};

}  // namespace osm

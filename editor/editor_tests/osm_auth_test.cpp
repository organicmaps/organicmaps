#include "testing/testing.hpp"

#include "editor/osm_auth.hpp"

using osm::OsmOAuth;
using osm::ClientToken;

namespace
{
constexpr char const * kTestServer = "http://188.166.112.124:3000";
constexpr char const * kConsumerKey = "QqwiALkYZ4Jd19lo1dtoPhcwGQUqMCMeVGIQ8Ahb";
constexpr char const * kConsumerSecret = "wi9HZKFoNYS06Yad5s4J0bfFo2hClMlH7pXaXWS3";
constexpr char const * kTestUser = "Testuser";
constexpr char const * kTestPassword = "testtest";
constexpr char const * kInvalidPassword = "123";
constexpr char const * kFacebookToken = "CAAYYoGXMFUcBAHZBpDFyFPFQroYRMtzdCzXVFiqKcZAZB44jKjzW8WWoaPWI4xxl9EK8INIuTZAkhpURhwSiyOIKoWsgbqZAKEKIKZC3IdlUokPOEuaUpKQzgLTUcYNLiqgJogjUTL1s7Myqpf8cf5yoxQm32cqKZAdozrdx2df4FMJBSF7h0dXI49M2WjCyjPcEKntC4LfQsVwrZBn8uStvUJBVGMTwNWkZD";
}  // namespace

UNIT_TEST(OSM_Auth_InvalidLogin)
{
  OsmOAuth auth(kConsumerKey, kConsumerSecret, kTestServer, kTestServer);
  ClientToken token;
  auto result = auth.AuthorizePassword(kTestUser, kInvalidPassword, token);
  TEST_EQUAL(result, OsmOAuth::AuthResult::FailLogin, ("invalid password"));
  TEST(!token.IsValid(), ("not authorized"));
}

UNIT_TEST(OSM_Auth_Login)
{
  OsmOAuth auth(kConsumerKey, kConsumerSecret, kTestServer, kTestServer);
  ClientToken token;
  auto result = auth.AuthorizePassword(kTestUser, kTestPassword, token);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ("login to test server"));
  TEST(token.IsValid(), ("authorized"));
  string const perm = auth.Request(token, "/permissions");
  TEST(perm.find("write_api") != string::npos, ("can write to api"));
}

UNIT_TEST(OSM_Auth_Facebook)
{
  OsmOAuth auth(kConsumerKey, kConsumerSecret, kTestServer, kTestServer);
  ClientToken token;
  auto result = auth.AuthorizeFacebook(kFacebookToken, token);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ("login via facebook"));
  TEST(token.IsValid(), ("authorized"));
  string const perm = auth.Request(token, "/permissions");
  TEST(perm.find("write_api") != string::npos, ("can write to api"));
}

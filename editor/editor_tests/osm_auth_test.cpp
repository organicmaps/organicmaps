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
  OsmOAuth::Response const perm = auth.Request(token, "/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::ResponseCode::OK, ("permission request ok"));
  TEST(perm.second.find("write_api") != string::npos, ("can write to api"));
}

UNIT_TEST(OSM_Auth_Facebook)
{
  OsmOAuth auth(kConsumerKey, kConsumerSecret, kTestServer, kTestServer);
  auto result = auth.AuthorizeFacebook(kFacebookToken);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ("login via facebook"));
  TEST(auth.IsAuthorized(), ("authorized"));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::ResponseCode::OK, ("permission with stored token request ok"));
  TEST(perm.second.find("write_api") != string::npos, ("can write to api"));
}

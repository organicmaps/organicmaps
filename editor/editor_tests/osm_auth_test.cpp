#include "testing/testing.hpp"

#include "editor/osm_auth.hpp"

using osm::OsmOAuth;
using osm::ClientToken;

namespace
{
constexpr char const * kIZTestUser = "Testuser";
constexpr char const * kIZTestPassword = "testtest";
constexpr char const * kIZInvalidPassword = "123";
constexpr char const * kFacebookToken = "CAAYYoGXMFUcBAHZBpDFyFPFQroYRMtzdCzXVFiqKcZAZB44jKjzW8WWoaPWI4xxl9EK8INIuTZAkhpURhwSiyOIKoWsgbqZAKEKIKZC3IdlUokPOEuaUpKQzgLTUcYNLiqgJogjUTL1s7Myqpf8cf5yoxQm32cqKZAdozrdx2df4FMJBSF7h0dXI49M2WjCyjPcEKntC4LfQsVwrZBn8uStvUJBVGMTwNWkZD";
}  // namespace

UNIT_TEST(OSM_Auth_InvalidLogin)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  auto const result = auth.AuthorizePassword(kIZTestUser, kIZInvalidPassword);
  TEST_EQUAL(result, OsmOAuth::AuthResult::FailLogin, ("invalid password"));
  TEST(!auth.IsAuthorized(), ("Should not be authorized."));
}

UNIT_TEST(OSM_Auth_Login)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  auto const result = auth.AuthorizePassword(kIZTestUser, kIZTestPassword);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ("login to test server"));
  TEST(auth.IsAuthorized(), ("Should be authorized."));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::ResponseCode::OK, ("permission request ok"));
  TEST(perm.second.find("write_api") != string::npos, ("can write to api"));
}

UNIT_TEST(OSM_Auth_Facebook)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  auto const result = auth.AuthorizeFacebook(kFacebookToken);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ("login via facebook"));
  TEST(auth.IsAuthorized(), ("Should be authorized."));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::ResponseCode::OK, ("permission with stored token request ok"));
  TEST(perm.second.find("write_api") != string::npos, ("can write to api"));
}

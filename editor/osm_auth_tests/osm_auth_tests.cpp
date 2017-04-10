#include "testing/testing.hpp"

#include "editor/osm_auth.hpp"

using osm::OsmOAuth;
using osm::TKeySecret;

namespace
{
constexpr char const * kIZTestUser = "Testuser";
constexpr char const * kIZTestPassword = "testtest";
constexpr char const * kIZInvalidPassword = "123";
constexpr char const * kIZForgotPasswordEmail = "test@example.com";
}  // namespace

UNIT_TEST(OSM_Auth_InvalidLogin)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kIZTestUser, kIZInvalidPassword), ());
  TEST_EQUAL(result, false, ("invalid password"));
  TEST(!auth.IsAuthorized(), ("Should not be authorized."));
}

UNIT_TEST(OSM_Auth_Login)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kIZTestUser, kIZTestPassword), ());
  TEST_EQUAL(result, true, ("login to test server"));
  TEST(auth.IsAuthorized(), ("Should be authorized."));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::HTTP::OK, ("permission request ok"));
  TEST_NOT_EQUAL(perm.second.find("write_api"), string::npos, ("can write to api"));
}

UNIT_TEST(OSM_Auth_ForgotPassword)
{
  OsmOAuth auth = OsmOAuth::IZServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.ResetPassword(kIZForgotPasswordEmail), ());
  TEST_EQUAL(result, true, ("Correct email"));
  TEST_NO_THROW(result = auth.ResetPassword("not@registered.email"), ());
  TEST_EQUAL(result, false, ("Incorrect email"));
}

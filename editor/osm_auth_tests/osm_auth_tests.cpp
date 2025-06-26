#include "testing/testing.hpp"

#include "platform/platform.hpp"
#include "editor/osm_auth.hpp"

#include <string>

namespace osm_auth
{
using osm::OsmOAuth;

char const * kValidOsmUser = "OrganicMapsTestUser";
char const * kValidOsmPassword = "12345678";
static constexpr char const * kInvalidOsmPassword = "123";
static constexpr char const * kForgotPasswordEmail = "osmtest1@organicmaps.app";

UNIT_TEST(OSM_Auth_InvalidLogin)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kValidOsmUser, kInvalidOsmPassword), ());
  TEST_EQUAL(result, false, ("invalid password"));
  TEST(!auth.IsAuthorized(), ("Should not be authorized."));
}

UNIT_TEST(OSM_Auth_Login)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kValidOsmUser, kValidOsmPassword), ());
  TEST_EQUAL(result, true, ("login to test server"));
  TEST(auth.IsAuthorized(), ("Should be authorized."));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::HTTP::OK, ("permission request ok"));
  TEST_NOT_EQUAL(perm.second.find("write_api"), std::string::npos, ("can write to api"));
}

/*
UNIT_TEST(OSM_Auth_ForgotPassword)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.ResetPassword(kForgotPasswordEmail), ());
  TEST_EQUAL(result, true, ("Correct email"));
  TEST_NO_THROW(result = auth.ResetPassword("not@registered.email"), ());
  TEST_EQUAL(result, false, ("Incorrect email"));
}
*/


UNIT_TEST(OAuth_Token_parse)
{
  osm::FindAuthenticityToken("");
  //TEST_EQUAL(perm.first, OsmOAuth::HTTP::OK, ("permission request ok"));
  //TEST_NOT_EQUAL(perm.second.find("write_api"), std::string::npos, ("can write to api"));
}

UNIT_TEST(OAuth_FindOauthCode) {
  auto code = osm::FindOauthCode("om://oauth2/osm/callback?redirect=true&code=befd_095315f197f4");
  TEST_EQUAL(code, "befd_095315f197f4", ("Invalid code"));

  code = osm::FindOauthCode("om://oauth2/osm/callback?code=45c023d6");
  TEST_EQUAL(code, "45c023d6", ("Invalid code"));

  code = osm::FindOauthCode("om://oauth2/osm/callback?the_code=cant_find");
  TEST_EQUAL(code, std::string{}, ("Code should not be found"));
}

UNIT_TEST(OAuth_FindAccessToken) {
  auto token = osm::FindAccessToken("{\"access_token\":\"vAK5XMBJeUDsF3xxFHt0\", \"token_type\":\"Bearer\", \"scope\":\"read_prefs\"}");
  TEST_EQUAL(token, "vAK5XMBJeUDsF3xxFHt0", ("Invalid access_token"));

  token = osm::FindAccessToken("{\"token_type\":\"Bearer\", \"scope\":\"read_prefs\", \"access_token\":\"W1xgY0CtTDz0klQGa4Yp\"}");
  TEST_EQUAL(token, "W1xgY0CtTDz0klQGa4Yp", ("Invalid access_token"));

  token = osm::FindAccessToken("{\"token_type\":\"Bearer\", \"scope\":\"read_prefs\", \"other_token\":\"9cMyxvsOrM\"}");
  TEST_EQUAL(token, std::string{}, ("access_token should not be found"));
}

}  // namespace osm_auth

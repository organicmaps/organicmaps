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

/*UNIT_TEST(OSM_Auth_InvalidLogin)
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
}*/

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
  auto token = osm::FindAuthenticityToken("/oauth2/authorize",
  "<div class=\"col-auto mx-1\"><form action=\"/oauth2/authorize\" accept-charset=\"UTF-8\" method=\"post\">"
  "  <input type=\"hidden\" name=\"authenticity_token\" value=\"J5senhh\" />\n"
  "  <input value=\"om://oauth2/osm/callback\" type=\"hidden\" name=\"redirect_uri\" />\n"
  "  <input type=\"submit\" name=\"commit\" value=\"Authorize\" class=\"btn btn-primary\" />\n"
  "</form></div>\n");

  TEST_EQUAL(token, "J5senhh", ("Invalid token"));

  // Two tokens. One for "Allow" other for "Deny". Always pick the first one.
  token = osm::FindAuthenticityToken("/oauth2/authorize",
  "<div class=\"col-auto mx-1\"><form action=\"/oauth2/authorize\" accept-charset=\"UTF-8\" method=\"post\">\n"
  "  <input type=\"hidden\" name=\"authenticity_token\" value=\"lRFbEQN0d2r7XeFq\" />\n"
  "  <input value=\"om://oauth2/osm/callback\" type=\"hidden\" name=\"redirect_uri\" />\n"
  "  <input type=\"submit\" name=\"commit\" value=\"Authorize\" class=\"btn btn-primary\" />\n"
  "</form></div>\n"
   "<div class=\"col-auto mx-1\"><form action=\"/oauth2/authorize\" accept-charset=\"UTF-8\" method=\"post\">\n"
   "  <input type=\"hidden\" name=\"_method\" value=\"delete\" />\n"
   "  <input type=\"hidden\" name=\"authenticity_token\" value=\"J5senhh\" />\n"
   "  <input value=\"om://oauth2/osm/callback\" type=\"hidden\" name=\"redirect_uri\" />\n"
   "  <input type=\"submit\" name=\"commit\" value=\"Deny\" class=\"btn btn-secondary\" />\n"
   "</form></div>\n");

  TEST_EQUAL(token, "lRFbEQN0d2r7XeFq", ("Invalid token"));

  // Three forms: one for language, two for OAuth
  token = osm::FindAuthenticityToken("/oauth2/authorize",
   "<div class=\"modal-body px-1\"><form action=\"/preferences/basic\" accept-charset=\"UTF-8\" method=\"post\">\n"
   "  <input type=\"hidden\" name=\"_method\" value=\"put\" />\n"
   "  <input type=\"hidden\" name=\"authenticity_token\" value=\"8h-snCINM3O\" />\n"
   "</form></div>\n"   "<div class=\"col-auto mx-1\"><form action=\"/oauth2/authorize\" accept-charset=\"UTF-8\" method=\"post\">\n"
   "  <input type=\"hidden\" name=\"authenticity_token\" value=\"8DDeqYcFHUIjw\" />\n"
   "  <input value=\"om://oauth2/osm/callback\" type=\"hidden\" name=\"redirect_uri\" />\n"
   "  <input type=\"submit\" name=\"commit\" value=\"Authorize\" class=\"btn btn-primary\" />\n"
   "</form></div>\n"
   "<div class=\"col-auto mx-1\"><form action=\"/oauth2/authorize\" accept-charset=\"UTF-8\" method=\"post\">\n"
   "  <input type=\"hidden\" name=\"_method\" value=\"delete\" />\n"
   "  <input type=\"hidden\" name=\"authenticity_token\" value=\"4RbveLjuvOXlok9Q\" />\n"
   "  <input value=\"om://oauth2/osm/callback\" type=\"hidden\" name=\"redirect_uri\" />\n"
   "  <input type=\"submit\" name=\"commit\" value=\"Deny\" class=\"btn btn-secondary\" />\n"
   "</form></div>\n");

  TEST_EQUAL(token, "8DDeqYcFHUIjw", ("Invalid token"));
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

  token = osm::FindAccessToken("{\"token_type\":\"Bearer\", \"scope\":\"read_prefs\", \"some_other_token\":\"9cMyxvsOrM\"}");
  TEST_EQUAL(token, std::string{}, ("access_token should not be found"));
}

}  // namespace osm_auth

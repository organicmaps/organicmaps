#include "editor/osm_auth.hpp"

namespace osm_auth_ios
{

enum class AuthorizationButtonType
{
  AuthorizationButtonTypeGoogle,
  AuthorizationButtonTypeFacebook,
  AuthorizationButtonTypeOSM
};

UIColor * AuthorizationButtonBackgroundColor(AuthorizationButtonType type);
void AuthorizationConfigButton(UIButton * btn, AuthorizationButtonType type);

// Deletes any stored credentials if called with empty key or secret.
void AuthorizationStoreCredentials(osm::TKeySecret const & keySecret);
BOOL AuthorizationHaveCredentials();
// Returns empty key and secret if user has not beed authorized.
osm::TKeySecret AuthorizationGetCredentials();

void AuthorizationSetNeedCheck(BOOL needCheck);
BOOL AuthorizationIsNeedCheck();
NSString * OSMUserName();

} // namespace osm_auth_ios

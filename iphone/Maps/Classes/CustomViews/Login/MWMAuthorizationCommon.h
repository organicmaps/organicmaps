#include "editor/osm_auth.hpp"

namespace osm_auth_ios
{

enum class AuthorizationButtonType
{
  AuthorizationButtonTypeGoogle,
  AuthorizationButtonTypeFacebook,
  AuthorizationButtonTypeOSM
};

// Deletes any stored credentials if called with empty key or secret.
void AuthorizationStoreCredentials(std::string const & oauthToken);
BOOL AuthorizationHaveOAuth1Credentials();
void AuthorizationClearOAuth1Credentials();
BOOL AuthorizationHaveCredentials();
void AuthorizationClearCredentials();
// Returns empty key and secret if user has not beed authorized.
std::string const AuthorizationGetCredentials();

void AuthorizationSetNeedCheck(BOOL needCheck);
BOOL AuthorizationIsNeedCheck();
/// Returns nil if not logged in.
NSString * OSMUserName();
/// Returns 0 if not logged in.
NSInteger OSMUserChangesetsCount();

}  // namespace osm_auth_ios

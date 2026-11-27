#pragma once

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, MWMOAuth2Result) {
  MWMOAuth2ResultSuccess,
  MWMOAuth2ResultFailure,
  MWMOAuth2ResultSuperseded
};

#ifdef __cplusplus
#include "editor/osm_auth.hpp"

namespace osm_auth_ios
{

enum class AuthorizationButtonType
{
  AuthorizationButtonTypeGoogle,
  AuthorizationButtonTypeFacebook,
  AuthorizationButtonTypeOSM
};

// Call on the main thread.
void AuthorizationStoreCredentials(std::string const & oauthToken);
// Starts the exchange on a background queue and completes on the main thread.
void AuthorizationStoreCredentialsWithOauth2Code(std::string const & oauth2Code, void (^completion)(MWMOAuth2Result));
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
#endif

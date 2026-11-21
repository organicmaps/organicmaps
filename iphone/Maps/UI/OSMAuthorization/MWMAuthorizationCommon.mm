#import "MWMAuthorizationCommon.h"
#import "MWMNetworkPolicy+UI.h"
#import "UIButton+RuntimeAttributes.h"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "editor/server_api.hpp"

#include <optional>

namespace osm_auth_ios
{
namespace
{

NSString * const kOSMRequestToken = @"OSMRequestToken";    // Unused after migration from OAuth1 to OAuth2
NSString * const kOSMRequestSecret = @"OSMRequestSecret";  // Unused after migration from OAuth1 to OAuth2
NSString * const kAuthNeedCheck = @"AuthNeedCheck";
NSString * const kOSMAuthToken = @"OSMAuthToken";
NSString * const kOSMUserName = @"UDOsmUserName";
NSString * const kOSMChangesetsCount = @"OSMUserChangesetsCount";

// Credential writes are serialized on the main thread. A newer write or logout
// invalidates a browser exchange that is still waiting on the server.
uint64_t g_credentialsRevision = 0;

std::optional<osm::UserPreferences> LoadOsmUserPreferences(std::string const & token)
{
  try
  {
    return osm::ServerApi06{osm::OsmOAuth::ServerAuth(token)}.GetUserPreferences();
  }
  catch (std::exception const &)
  {
    LOG(LWARNING, ("Can't load user preferences from OSM server"));
    return std::nullopt;
  }
}

void StoreCredentials(std::string const & token, std::optional<osm::UserPreferences> const & prefs)
{
  ASSERT(NSThread.isMainThread, ());
  ++g_credentialsRevision;
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if (prefs)
  {
    [ud setObject:@(prefs->m_displayName.c_str()) forKey:kOSMUserName];
    // To also see # of edits when offline.
    [ud setInteger:prefs->m_changesets forKey:kOSMChangesetsCount];
  }
  else if (token != AuthorizationGetCredentials())
  {
    [ud removeObjectForKey:kOSMUserName];
    [ud removeObjectForKey:kOSMChangesetsCount];
  }
  [ud setObject:@(token.c_str()) forKey:kOSMAuthToken];
}

std::string ExchangeOAuth2Code(std::string const & code)
{
  try
  {
    return osm::OsmOAuth::ServerAuth().FinishAuthorization(code);
  }
  catch (osm::OsmOAuth::OsmOAuthException const &)
  {
    // OAuth errors can include the server response. Keep it out of the log.
    LOG(LWARNING, ("OpenStreetMap token exchange failed"));
    return {};
  }
}
}  // namespace

void AuthorizationStoreCredentials(std::string const & oauthToken)
{
  StoreCredentials(oauthToken, LoadOsmUserPreferences(oauthToken));
}

void AuthorizationStoreCredentialsWithOauth2Code(std::string const & oauth2Code, void (^completion)(MWMOAuth2Result))
{
  ASSERT(NSThread.isMainThread, ());
  auto const revision = g_credentialsRevision;
  auto const code = oauth2Code;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    auto const token = ExchangeOAuth2Code(code);
    auto const prefs = token.empty() ? std::nullopt : LoadOsmUserPreferences(token);
    dispatch_async(dispatch_get_main_queue(), ^{
      if (revision != g_credentialsRevision)
        completion(MWMOAuth2ResultSuperseded);
      else if (token.empty())
        completion(MWMOAuth2ResultFailure);
      else
      {
        StoreCredentials(token, prefs);
        completion(MWMOAuth2ResultSuccess);
      }
    });
  });
}

BOOL AuthorizationHaveOAuth1Credentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  return requestToken.length && requestSecret.length;
}

void AuthorizationClearOAuth1Credentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud removeObjectForKey:kOSMRequestToken];
  [ud removeObjectForKey:kOSMRequestSecret];
}

BOOL AuthorizationHaveCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * oauthToken = [ud stringForKey:kOSMAuthToken];
  return oauthToken.length;
}

void AuthorizationClearCredentials()
{
  ASSERT(NSThread.isMainThread, ());
  ++g_credentialsRevision;
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud removeObjectForKey:kOSMAuthToken];
  [ud removeObjectForKey:kOSMRequestToken];
  [ud removeObjectForKey:kOSMRequestSecret];
  [ud removeObjectForKey:kOSMUserName];
  [ud removeObjectForKey:kOSMChangesetsCount];
}

std::string const AuthorizationGetCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * oauthToken = [ud stringForKey:kOSMAuthToken];
  if (oauthToken)
    return std::string(oauthToken.UTF8String);
  return {};
}

void AuthorizationSetNeedCheck(BOOL needCheck)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:needCheck forKey:kAuthNeedCheck];
}

BOOL AuthorizationIsNeedCheck()
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kAuthNeedCheck];
}

NSString * OSMUserName()
{
  return [NSUserDefaults.standardUserDefaults stringForKey:kOSMUserName];
}

NSInteger OSMUserChangesetsCount()
{
  __block NSInteger count = -1;
  [[MWMNetworkPolicy sharedPolicy] callOnlineApi:^(BOOL permitted) {
    if (permitted)
      if (auto const prefs = LoadOsmUserPreferences(AuthorizationGetCredentials()))
        count = prefs->m_changesets;
  }];

  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if (count >= 0)
  {
    [ud setInteger:count forKey:kOSMChangesetsCount];
    return count;
  }
  return [ud integerForKey:kOSMChangesetsCount];
}

}  // namespace osm_auth_ios

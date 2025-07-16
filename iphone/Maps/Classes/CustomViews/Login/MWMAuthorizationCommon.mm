#import "MWMAuthorizationCommon.h"
#import "MWMNetworkPolicy+UI.h"
#import "UIButton+RuntimeAttributes.h"

#include "base/logging.hpp"
#include "editor/server_api.hpp"

namespace osm_auth_ios
{

NSString * const kOSMRequestToken = @"OSMRequestToken";    // Unused after migration from OAuth1 to OAuth2
NSString * const kOSMRequestSecret = @"OSMRequestSecret";  // Unused after migration from OAuth1 to OAuth2
NSString * const kAuthNeedCheck = @"AuthNeedCheck";
NSString * const kOSMAuthToken = @"OSMAuthToken";
NSString * const kOSMUserName = @"UDOsmUserName";
NSString * const kOSMChangesetsCount = @"OSMUserChangesetsCount";

BOOL LoadOsmUserPreferences(osm::UserPreferences & prefs)
{
  __block BOOL success = false;
  dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    try
    {
      osm::ServerApi06 const api{osm::OsmOAuth::ServerAuth(AuthorizationGetCredentials())};
      prefs = api.GetUserPreferences();
      success = true;
    }
    catch (std::exception const & ex)
    {
      LOG(LWARNING, ("Can't load user preferences from OSM server:", ex.what()));
    }
  });
  return success;
}

void AuthorizationStoreCredentials(std::string const & oauthToken)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:@(oauthToken.c_str()) forKey:kOSMAuthToken];
  osm::UserPreferences prefs;
  if (LoadOsmUserPreferences(prefs))
  {
    [ud setObject:@(prefs.m_displayName.c_str()) forKey:kOSMUserName];
    // To also see # of edits when offline.
    [ud setInteger:prefs.m_changesets forKey:kOSMChangesetsCount];
  }
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
      if (osm::UserPreferences prefs; YES == LoadOsmUserPreferences(prefs))
        count = prefs.m_changesets;
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

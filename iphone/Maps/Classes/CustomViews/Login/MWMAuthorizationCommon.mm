#import "MWMAuthorizationCommon.h"
#import "MWMNetworkPolicy+UI.h"
#import "UIButton+RuntimeAttributes.h"

#include "base/logging.hpp"
#include "editor/server_api.hpp"

namespace osm_auth_ios
{

NSString * const kOSMRequestToken = @"OSMRequestToken";
NSString * const kOSMRequestSecret = @"OSMRequestSecret";
NSString * const kAuthNeedCheck = @"AuthNeedCheck";
NSString * const kOSMUserName = @"UDOsmUserName";
NSString * const kOSMChangesetsCount = @"OSMUserChangesetsCount";

BOOL LoadOsmUserPreferences(osm::UserPreferences & prefs)
{
  __block BOOL success = false;
  dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    try
    {
      osm::ServerApi06 const api {osm::OsmOAuth::ServerAuth(AuthorizationGetCredentials())};
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

void AuthorizationStoreCredentials(osm::KeySecret const & keySecret)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:@(keySecret.first.c_str()) forKey:kOSMRequestToken];
  [ud setObject:@(keySecret.second.c_str()) forKey:kOSMRequestSecret];
  osm::UserPreferences prefs;
  if (LoadOsmUserPreferences(prefs)) {
    [ud setObject:@(prefs.m_displayName.c_str()) forKey:kOSMUserName];
    // To also see # of edits when offline.
    [ud setInteger:prefs.m_changesets forKey:kOSMChangesetsCount];
  }
  [ud synchronize];
}

BOOL AuthorizationHaveCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  return requestToken.length && requestSecret.length;
}

void AuthorizationClearCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud removeObjectForKey:kOSMRequestToken];
  [ud removeObjectForKey:kOSMRequestSecret];
  [ud removeObjectForKey:kOSMUserName];
  [ud removeObjectForKey:kOSMChangesetsCount];
  [ud synchronize];
}

osm::KeySecret AuthorizationGetCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  if (requestToken && requestSecret)
    return osm::KeySecret(requestToken.UTF8String, requestSecret.UTF8String);
  return {};
}

void AuthorizationSetNeedCheck(BOOL needCheck)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:needCheck forKey:kAuthNeedCheck];
  [ud synchronize];
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
    [ud synchronize];
    return count;
  }
  return [ud integerForKey:kOSMChangesetsCount];
}

} // namespace osm_auth_ios

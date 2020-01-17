#import "MWMAuthorizationCommon.h"
#import "UIButton+RuntimeAttributes.h"

#include "base/logging.hpp"
#include "editor/server_api.hpp"

namespace osm_auth_ios
{

NSString * const kOSMRequestToken = @"OSMRequestToken";
NSString * const kOSMRequestSecret = @"OSMRequestSecret";
NSString * const kAuthNeedCheck = @"AuthNeedCheck";
NSString * const kOSMUserName = @"UDOsmUserName";

void SetOSMUserNameWithCredentials(osm::KeySecret const & keySecret)
{
  using namespace osm;
  dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    ServerApi06 const api {OsmOAuth::ServerAuth(keySecret)};
    try
    {
      NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
      [ud setObject:@(api.GetUserPreferences().m_displayName.c_str()) forKey:kOSMUserName];
      [ud synchronize];
    }
    catch (std::exception const & ex)
    {
       LOG(LWARNING, ("Can't load user preferences from OSM server:", ex.what()));
    }
  });
} // namespace osm_auth_ios

void SetEmptyOSMUserName()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:nil forKey:kOSMUserName];
  [ud synchronize];
}

void AuthorizationStoreCredentials(osm::KeySecret const & keySecret)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if (keySecret.first.empty() || keySecret.second.empty())
  {
    [ud removeObjectForKey:kOSMRequestToken];
    [ud removeObjectForKey:kOSMRequestSecret];
    SetEmptyOSMUserName();
  }
  else
  {
    [ud setObject:@(keySecret.first.c_str()) forKey:kOSMRequestToken];
    [ud setObject:@(keySecret.second.c_str()) forKey:kOSMRequestSecret];
    SetOSMUserNameWithCredentials(keySecret);
  }
  [ud synchronize];
}

BOOL AuthorizationHaveCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  return requestToken && requestSecret;
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

} // namespace osm_auth_ios

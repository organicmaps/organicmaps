#import "MWMCommon.h"
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

void SetOSMUserNameWithCredentials(osm::TKeySecret const & keySecret)
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

UIColor * AuthorizationButtonTextColor(AuthorizationButtonType type)
{
  switch (type)
  {
  case AuthorizationButtonType::AuthorizationButtonTypeGoogle: return UIColor.blackColor;
  case AuthorizationButtonType::AuthorizationButtonTypeFacebook: return UIColor.whiteColor;
  case AuthorizationButtonType::AuthorizationButtonTypeOSM: return [UIColor white];
  }
  return UIColor.clearColor;
}

UIColor * AuthorizationButtonBackgroundColor(AuthorizationButtonType type)
{
  switch (type)
  {
  case AuthorizationButtonType::AuthorizationButtonTypeGoogle: return UIColor.whiteColor;
  case AuthorizationButtonType::AuthorizationButtonTypeFacebook:
    return [UIColor colorWithRed:72. / 255. green:97. / 255. blue:163. / 255. alpha:1.];
  case AuthorizationButtonType::AuthorizationButtonTypeOSM: return [UIColor linkBlue];
  }
  return UIColor.clearColor;
}

void AuthorizationConfigButton(UIButton * btn, AuthorizationButtonType type)
{
  UIColor * txtCol = AuthorizationButtonTextColor(type);
  UIColor * bgCol = AuthorizationButtonBackgroundColor(type);

  CGFloat const highlightedAlpha = 0.3;
  [btn setTitleColor:txtCol forState:UIControlStateNormal];
  [btn setTitleColor:[txtCol colorWithAlphaComponent:highlightedAlpha] forState:UIControlStateHighlighted];

  [btn setBackgroundColor:bgCol forState:UIControlStateNormal];
  [btn setBackgroundColor:[bgCol colorWithAlphaComponent:highlightedAlpha] forState:UIControlStateHighlighted];
}

void AuthorizationStoreCredentials(osm::TKeySecret const & keySecret)
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

osm::TKeySecret AuthorizationGetCredentials()
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  if (requestToken && requestSecret)
    return osm::TKeySecret(requestToken.UTF8String, requestSecret.UTF8String);
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

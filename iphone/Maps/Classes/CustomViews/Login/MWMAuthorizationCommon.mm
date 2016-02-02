#import "MWMAuthorizationCommon.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

namespace osm_auth_ios
{

NSString * const kOSMRequestToken = @"OSMRequestToken";
NSString * const kOSMRequestSecret = @"OSMRequestSecret";
NSString * const kAuthUserSkip = @"AuthUserSkip";
NSString * const kAuthNeedCheck = @"AuthNeedCheck";

UIColor * AuthorizationButtonTextColor(AuthorizationButtonType type)
{
  switch (type)
  {
    case AuthorizationButtonType::AuthorizationButtonTypeGoogle:
      return [UIColor blackColor];
    case AuthorizationButtonType::AuthorizationButtonTypeFacebook:
      return [UIColor whiteColor];
    case AuthorizationButtonType::AuthorizationButtonTypeOSM:
      return [UIColor white];
  }
  return [UIColor clearColor];
}

UIColor * AuthorizationButtonBackgroundColor(AuthorizationButtonType type)
{
  switch (type)
  {
    case AuthorizationButtonType::AuthorizationButtonTypeGoogle:
      return [UIColor whiteColor];
    case AuthorizationButtonType::AuthorizationButtonTypeFacebook:
      return [UIColor colorWithRed:72. / 255. green:97. / 255. blue:163. / 255. alpha:1.];
    case AuthorizationButtonType::AuthorizationButtonTypeOSM:
      return [UIColor linkBlue];
  }
  return [UIColor clearColor];
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
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  if (keySecret.first.empty() || keySecret.second.empty())
  {
    [ud removeObjectForKey:kOSMRequestToken];
    [ud removeObjectForKey:kOSMRequestSecret];
  }
  else
  {
    [ud setObject:@(keySecret.first.c_str()) forKey:kOSMRequestToken];
    [ud setObject:@(keySecret.second.c_str()) forKey:kOSMRequestSecret];
  }
  [ud synchronize];
}

BOOL AuthorizationHaveCredentials()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  return requestToken && requestSecret;
}

osm::TKeySecret AuthorizationGetCredentials()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  if (requestToken && requestSecret)
    return osm::TKeySecret(requestToken.UTF8String, requestSecret.UTF8String);
  return {};
}

void AuthorizationSetUserSkip()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:[NSDate date] forKey:kAuthUserSkip];
  [ud synchronize];
}

BOOL AuthorizationIsUserSkip()
{
  return [[NSUserDefaults standardUserDefaults] objectForKey:kAuthUserSkip] != nil;
}

void AuthorizationSetNeedCheck(BOOL needCheck)
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:needCheck forKey:kAuthNeedCheck];
  [ud synchronize];
}

BOOL AuthorizationIsNeedCheck()
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:kAuthNeedCheck];
}

} // namespace osm_auth_ios

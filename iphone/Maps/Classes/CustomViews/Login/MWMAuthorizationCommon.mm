#import "MWMAuthorizationCommon.h"
#import "UIButton+RuntimeAttributes.h"

namespace
{
NSString * const kOSMRequestToken = @"OSMRequestToken";
NSString * const kOSMRequestSecret = @"OSMRequestSecret";
NSString * const kAuthUserSkip = @"AuthUserSkip";
NSString * const kAuthNeedCheck = @"AuthNeedCheck";
}  // namespace

UIColor * MWMAuthorizationButtonTextColor(MWMAuthorizationButtonType type)
{
  switch (type)
  {
    case MWMAuthorizationButtonTypeGoogle:
      return [UIColor blackColor];
    case MWMAuthorizationButtonTypeFacebook:
    case MWMAuthorizationButtonTypeOSM:
      return [UIColor whiteColor];
  }
  return [UIColor clearColor];
}

UIColor * MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonType type)
{
  switch (type)
  {
    case MWMAuthorizationButtonTypeGoogle:
      return [UIColor whiteColor];
    case MWMAuthorizationButtonTypeFacebook:
      return [UIColor colorWithRed:72. / 255. green:97. / 255. blue:163. / 255. alpha:1.];
    case MWMAuthorizationButtonTypeOSM:
      return [UIColor colorWithRed:30. / 255. green:150. / 255. blue:240. / 255. alpha:1.];
  }
  return [UIColor clearColor];
}

void MWMAuthorizationConfigButton(UIButton * btn, MWMAuthorizationButtonType type)
{
  UIColor * txtCol = MWMAuthorizationButtonTextColor(type);
  UIColor * bgCol = MWMAuthorizationButtonBackgroundColor(type);

  [btn setTitleColor:txtCol forState:UIControlStateNormal];
  [btn setTitleColor:bgCol forState:UIControlStateHighlighted];

  [btn setBackgroundColor:bgCol forState:UIControlStateNormal];
  [btn setBackgroundColor:[UIColor clearColor] forState:UIControlStateHighlighted];

  btn.layer.borderColor = bgCol.CGColor;
}

void MWMAuthorizationStoreCredentials(osm::TKeySecret const & keySecret)
{
  NSString * requestToken = @(keySecret.first.c_str());
  NSString * requestSecret = @(keySecret.second.c_str());
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:requestToken forKey:kOSMRequestToken];
  [ud setObject:requestSecret forKey:kOSMRequestSecret];
}

BOOL MWMAuthorizationHaveCredentials()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  return requestToken && requestSecret;
}

void MWMAuthorizationSetUserSkip()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:[NSDate date] forKey:kAuthUserSkip];
}

BOOL MWMAuthorizationIsUserSkip()
{
  return [[NSUserDefaults standardUserDefaults] objectForKey:kAuthUserSkip] != nil;
}

void MWMAuthorizationSetNeedCheck(BOOL needCheck)
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:needCheck forKey:kAuthNeedCheck];
  [ud synchronize];
}

BOOL MWMAuthorizationIsNeedCheck()
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:kAuthNeedCheck];
}

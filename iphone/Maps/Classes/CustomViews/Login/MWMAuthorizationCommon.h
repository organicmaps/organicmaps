
static NSString * const kOSMUsernameKey = @"OSMUsernameKey";
static NSString * const kOSMPasswordKey = @"OSMPasswordKey";

typedef NS_OPTIONS(NSUInteger, MWMAuthorizationButtonType)
{
  MWMAuthorizationButtonTypeGoogle,
  MWMAuthorizationButtonTypeFacebook,
  MWMAuthorizationButtonTypeOSM
};

UIColor * MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonType type);
void MWMAuthorizationConfigButton(UIButton * btn, MWMAuthorizationButtonType type);


static NSString * const kOSMRequestToken = @"OSMRequestToken";
static NSString * const kOSMRequestSecret = @"OSMRequestSecret";

typedef NS_OPTIONS(NSUInteger, MWMAuthorizationButtonType)
{
  MWMAuthorizationButtonTypeGoogle,
  MWMAuthorizationButtonTypeFacebook,
  MWMAuthorizationButtonTypeOSM
};

UIColor * MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonType type);
void MWMAuthorizationConfigButton(UIButton * btn, MWMAuthorizationButtonType type);

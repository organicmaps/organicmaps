#import "MWMAuthorizationCommon.h"
#import "UIButton+RuntimeAttributes.h"

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
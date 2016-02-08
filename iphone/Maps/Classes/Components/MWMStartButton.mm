#import "MWMStartButton.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMStartButton

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlue]] forState:UIControlStateNormal];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlueDark]] forState:UIControlStateHighlighted];
}

@end

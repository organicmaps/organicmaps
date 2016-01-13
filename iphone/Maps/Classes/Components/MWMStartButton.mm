#import "MWMStartButton.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMStartButton

- (void)refresh
{
  [super refresh];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlue]] forState:UIControlStateNormal];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlueDark]] forState:UIControlStateHighlighted];
}

@end

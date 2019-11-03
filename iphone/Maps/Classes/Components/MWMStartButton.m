#import "MWMStartButton.h"

@implementation MWMStartButton

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlue]] forState:UIControlStateNormal];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor linkBlueHighlighted]] forState:UIControlStateHighlighted];
}

@end

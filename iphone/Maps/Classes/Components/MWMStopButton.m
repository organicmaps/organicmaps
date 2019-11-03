#import "MWMStopButton.h"

@implementation MWMStopButton

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor buttonRed]]
                  forState:UIControlStateNormal];
  [self setBackgroundImage:[UIImage imageWithColor:[UIColor buttonRedHighlighted]]
                  forState:UIControlStateHighlighted];
}

@end

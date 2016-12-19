#import "MWMCommon.h"
#import "UIButton+Orientation.h"

@implementation UIButton (Orientation)

- (void)matchInterfaceOrientation
{
  if (isInterfaceRightToLeft())
    self.imageView.transform = CGAffineTransformMakeScale(-1, 1);
}

@end

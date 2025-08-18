#import "UIButton+Orientation.h"

#import <CoreApi/MWMCommon.h>

@implementation UIButton (Orientation)

- (void)matchInterfaceOrientation
{
  if (isInterfaceRightToLeft())
    self.imageView.transform = CGAffineTransformMakeScale(-1, 1);
}

@end

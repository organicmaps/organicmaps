#import "UIButton+Orientation.h"

#import <CoreApi/CoreApi.h>

@implementation UIButton (Orientation)

- (void)matchInterfaceOrientation
{
  if (isInterfaceRightToLeft())
    self.imageView.transform = CGAffineTransformMakeScale(-1, 1);
}

@end

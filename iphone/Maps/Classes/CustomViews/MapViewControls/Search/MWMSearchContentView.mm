#import "MWMSearchContentView.h"

@implementation MWMSearchContentView

- (void)layoutSubviews
{
  [self.subviews enumerateObjectsUsingBlock:^(UIView * view, NSUInteger idx, BOOL * stop)
  {
    view.frame = self.bounds;
  }];
  [super layoutSubviews];
}

@end

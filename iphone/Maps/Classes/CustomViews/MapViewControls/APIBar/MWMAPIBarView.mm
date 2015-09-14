#import "MWMAPIBarView.h"

@implementation MWMAPIBarView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  return self;
}

- (void)layoutSubviews
{
  self.frame = {{}, {self.superview.bounds.size.width, 20.0}};
  [super layoutSubviews];
}

@end

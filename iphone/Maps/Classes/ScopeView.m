
#import "ScopeView.h"
#import "UIKitCategories.h"

@implementation ScopeView

- (instancetype)initWithFrame:(CGRect)frame segmentedControl:(UISegmentedControl *)segmentedControl
{
  self = [super initWithFrame:frame];

  [self addSubview:segmentedControl];

  _segmentedControl = segmentedControl;

  return self;
}

- (void)layoutSubviews
{
  self.segmentedControl.center = CGPointMake(self.width / 2, self.height / 2);
}

@end

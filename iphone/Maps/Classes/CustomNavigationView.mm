#import "CustomNavigationView.h"

@implementation CustomNavigationView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void)layoutSubviews
{
  UINavigationBar * navBar = (UINavigationBar *)[self.subviews objectAtIndex:0];
  [navBar sizeToFit];
  [navBar setNeedsDisplay];

  UIView * table = [self.subviews objectAtIndex:1];
  CGRect rTable;
  rTable.origin = CGPointMake(navBar.frame.origin.x, navBar.frame.origin.y + navBar.frame.size.height);
  rTable.size = self.bounds.size;
  rTable.size.height -= navBar.bounds.size.height;
  table.frame = rTable;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Drawing code
}
*/

@end

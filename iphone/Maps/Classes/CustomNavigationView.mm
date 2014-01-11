#import "CustomNavigationView.h"

@implementation CustomNavigationView

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

@end

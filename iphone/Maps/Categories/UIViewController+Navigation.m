#import "UIButton+Orientation.h"
#import "UIViewController+Navigation.h"

static CGFloat const kButtonExtraWidth = 16.0;

@implementation UIViewController (Navigation)

- (UIBarButtonItem *)negativeSpacer
{
  UIBarButtonItem * spacer =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace
                                                    target:nil
                                                    action:nil];
  spacer.width = -kButtonExtraWidth;
  return spacer;
}

- (UIBarButtonItem *)buttonWithImage:(UIImage *)image action:(SEL)action
{
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width + kButtonExtraWidth, image.size.height)];
  [button setImage:image forState:UIControlStateNormal];
  [button matchInterfaceOrientation];
  [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
  return [[UIBarButtonItem alloc] initWithCustomView:button];
}

- (NSArray<UIBarButtonItem *> *)alignedNavBarButtonItems:(NSArray<UIBarButtonItem *> *)items
{
  return [@[ [self negativeSpacer] ] arrayByAddingObjectsFromArray:items];
}

- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }

@end

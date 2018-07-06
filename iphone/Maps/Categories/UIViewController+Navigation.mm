#import "MWMCommon.h"
#import "UIButton+Orientation.h"
#import "UIViewController+Navigation.h"

namespace
{
CGFloat constexpr kButtonExtraWidth = 16.0;
}  // namespace

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
  CGSize const buttonSize = {image.size.width + kButtonExtraWidth, image.size.height};
  UIButton * button = [[UIButton alloc] initWithFrame:{{}, buttonSize}];
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

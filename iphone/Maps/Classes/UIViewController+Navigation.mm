#import "Common.h"
#import "UIViewController+Navigation.h"

namespace
{
CGFloat constexpr kButtonExtraWidth = 16.0;
}

@implementation UIViewController (Navigation)

- (UIBarButtonItem *)negativeSpacer
{
  UIBarButtonItem * spacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace target:nil action:nil];
  spacer.width = -kButtonExtraWidth;
  return spacer;
}

- (UIBarButtonItem *)backButton
{
  UIImage * backImage = [UIImage imageNamed:@"ic_nav_bar_back"];
  UIImage * highlightedImage = [UIImage imageNamed:@"ic_nav_bar_back_press"];
  CGSize const buttonSize = {backImage.size.width + kButtonExtraWidth, backImage.size.height};
  UIButton * button = [[UIButton alloc] initWithFrame:{{}, buttonSize}];
  [button setImage:backImage forState:UIControlStateNormal];
  [button setImage:highlightedImage forState:UIControlStateHighlighted];
  [button addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  return [[UIBarButtonItem alloc] initWithCustomView:button];
}

- (void)showBackButton
{
  self.navigationItem.leftBarButtonItems = @[[self negativeSpacer], [self backButton]];
}

- (void)backTap
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (UIStoryboard *)mainStoryboard
{
  return [UIStoryboard storyboardWithName:@"Mapsme" bundle:nil];
}

@end

#import "Common.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (UIButton *)backButton
{
  UIImage * backImage = [UIImage imageNamed:@"ic_nav_bar_back"];
  UIButton * button = [[UIButton alloc] initWithFrame:{{}, backImage.size}];
  [button setImage:backImage forState:UIControlStateNormal];
  [button addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  button.imageEdgeInsets = {0, -16, 0, 0};
  return button;
}

- (void)showBackButton
{
  self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:[self backButton]];
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

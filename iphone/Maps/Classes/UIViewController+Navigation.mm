#import "Common.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (UIButton *)backButton
{
  UIImage * backImage = [UIImage imageNamed:@"ic_nav_bar_back"];
  CGFloat const imageSide = backImage.size.width;
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0., 0., imageSide, imageSide)];
  [button setImage:backImage forState:UIControlStateNormal];
  [button addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  button.imageEdgeInsets = UIEdgeInsetsMake(0., -32, 0., 0.);
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

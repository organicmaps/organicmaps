#import "Common.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (void)showBackButton
{
  UIImage * backImage = [UIImage imageNamed:@"NavigationBarBackButton"];
  CGFloat const imageSide = backImage.size.width;
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0., 0., imageSide, imageSide)];
  [button setImage:backImage forState:UIControlStateNormal];
  [button addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  button.imageEdgeInsets = UIEdgeInsetsMake(0., -imageSide, 0., 0.);
  UIBarButtonItem * leftItem = [[UIBarButtonItem alloc] initWithCustomView:button];
  self.navigationItem.leftBarButtonItem = leftItem;
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

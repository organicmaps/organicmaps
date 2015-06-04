#import "Common.h"
#import "UIKitCategories.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (void)showBackButton
{
  UIButton * backButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 56, 44)];
  [backButton addTarget:self action:@selector(backButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  [backButton setImage:[UIImage imageNamed:@"NavigationBarBackButton"] forState:UIControlStateNormal];

  UIBarButtonItem * space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace target:nil action:nil];
  space.width = isIOSVersionLessThan(7) ? -8 : -16;
  UIBarButtonItem * item = [[UIBarButtonItem alloc] initWithCustomView:backButton];
  self.navigationItem.leftBarButtonItems = @[space, item];
}

- (void)backButtonPressed:(id)sender
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (UIStoryboard *)mainStoryboard
{
  NSString * name = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? @"Main_iPad" : @"Main_iPhone";
  return [UIStoryboard storyboardWithName:name bundle:nil];
}

@end

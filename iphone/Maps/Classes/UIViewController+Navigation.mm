#import "Common.h"
#import "UIKitCategories.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (void)showBackButton
{
  UIBarButtonItem * leftItem = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"NavigationBarBackButton"] style:UIBarButtonItemStylePlain target:self action:@selector(backButtonPressed:)];
  self.navigationItem.leftBarButtonItem = leftItem;
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

#import "Common.h"
#import "MWMSegue.h"

@implementation MWMSegue

+ (void)segueFrom:(UIViewController *)source to:(UIViewController *)destination
{
  [[[MWMSegue alloc] initWithIdentifier:@"" source:source destination:destination] perform];
}

- (void)perform
{
  UINavigationController * nc = self.sourceViewController.navigationController;
  UIViewController * dvc = self.destinationViewController;
  if (isIOS7)
  {
    if ([dvc isMemberOfClass:[UINavigationController class]])
      [nc presentViewController:dvc animated:YES completion:nil];
    else
      [nc pushViewController:dvc animated:YES];
  }
  else
  {
    [nc showViewController:dvc sender:nil];
  }
}

@end

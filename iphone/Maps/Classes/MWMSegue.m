#import "MWMSegue.h"

@implementation MWMSegue

+ (void)segueFrom:(UIViewController *)source to:(UIViewController *)destination
{
  [[[self alloc] initWithIdentifier:@"" source:source destination:destination] perform];
}

- (void)perform
{
  UINavigationController * nc = self.sourceViewController.navigationController;
  UIViewController * dvc = self.destinationViewController;
  [nc showViewController:dvc sender:nil];
}

@end

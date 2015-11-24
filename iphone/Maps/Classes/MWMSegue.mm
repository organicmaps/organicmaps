#import "Common.h"
#import "MWMSegue.h"

@implementation MWMSegue

- (void)perform
{
  if (isIOSVersionLessThan(8))
    [self.sourceViewController.navigationController pushViewController:self.destinationViewController animated:YES];
  else
    [self.sourceViewController.navigationController showViewController:self.destinationViewController sender:nil];
}

@end

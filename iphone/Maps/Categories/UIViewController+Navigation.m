#import "UIButton+Orientation.h"
#import "UIViewController+Navigation.h"

@implementation UIViewController (Navigation)

- (void)goBack
{
  [self.navigationController popViewControllerAnimated:YES];
}

@end

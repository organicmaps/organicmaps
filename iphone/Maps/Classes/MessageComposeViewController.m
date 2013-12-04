
#import "MessageComposeViewController.h"
#import "UIKitCategories.h"

@implementation MessageComposeViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
    self.navigationBar.tintColor = [UIColor whiteColor];
}

@end

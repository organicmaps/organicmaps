
#import "MailComposeViewController.h"
#import "UIKitCategories.h"

@implementation MailComposeViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
    self.navigationBar.tintColor = [UIColor whiteColor];
}

@end


#import "MailComposeViewController.h"
#import "UIKitCategories.h"

@interface MailComposeViewController ()

@end

@implementation MailComposeViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
    self.navigationBar.tintColor = [UIColor whiteColor];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

@end

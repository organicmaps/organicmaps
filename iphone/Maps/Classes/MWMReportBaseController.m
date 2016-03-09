#import "MWMReportBaseController.h"

@implementation MWMReportBaseController

- (void)configNavBar
{
  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"send")
                                            style:UIBarButtonItemStylePlain target:self action:@selector(send)];
}

- (void)send
{
  [self.navigationController popToRootViewControllerAnimated:YES];
}

@end

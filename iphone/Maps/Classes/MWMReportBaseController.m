#import "MWMReportBaseController.h"

@implementation MWMReportBaseController

- (void)configNavBar
{
  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"editor_report_problem_send_button")
                                            style:UIBarButtonItemStylePlain target:self action:@selector(send)];
}

- (void)send
{
  [self.navigationController popToRootViewControllerAnimated:YES];
}

@end

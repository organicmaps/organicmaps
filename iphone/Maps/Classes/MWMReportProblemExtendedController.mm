#import "MWMReportProblemExtendedController.h"

@interface MWMReportProblemExtendedController ()

@property (weak, nonatomic) IBOutlet UITextView * textView;

@end

@implementation MWMReportProblemExtendedController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
}

- (void)configNavBar
{
  [super configNavBar];
  self.title = L(@"editor_report_problem_title");
}

#pragma mark - UITableView

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  NSAssert(section == 0, @"Invalid section!");
  return [NSString stringWithFormat:@"%@\n\n%@", L(@"editor_report_problem_desription_1"),
                                               L(@"editor_report_problem_desription_2")];
}

@end

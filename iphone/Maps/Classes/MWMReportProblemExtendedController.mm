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
  self.title = L(@"other");
}

#pragma mark - UITableView

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  NSAssert(section == 0, @"Invalid section!");
  return L(@"report_problem_extended_description");
}

@end

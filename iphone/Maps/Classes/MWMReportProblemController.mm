#import "MWMReportProblemController.h"
#import "SelectableCell.h"

@interface MWMReportProblemController ()

@property (weak, nonatomic) IBOutlet SelectableCell * placeDoesntExistCell;
@property (nonatomic) BOOL isCellSelected;

@end

@implementation MWMReportProblemController

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

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [cell setSelected:NO animated:YES];

  if (indexPath.row > 0)
    return;

  self.isCellSelected = !self.isCellSelected;
  self.placeDoesntExistCell.accessoryType =  self.isCellSelected ? UITableViewCellAccessoryCheckmark :
                                                                   UITableViewCellAccessoryNone;
}

@end

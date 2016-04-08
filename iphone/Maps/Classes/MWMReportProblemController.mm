#import "MWMReportProblemController.h"
#import "SelectableCell.h"

namespace
{
 string const kAmenityDoesntExist = "The amenity has gone or never existed. This is an auto-generated note from MAPS.ME application: a user reports a POI that is visible on a map (which can be a month old), but cannot be found on the ground.";
}

@interface MWMReportProblemController ()

@property (weak, nonatomic) IBOutlet SelectableCell * placeDoesntExistCell;
@property (nonatomic) BOOL isCellSelected;

@end

@implementation MWMReportProblemController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.placeDoesntExistCell.accessoryType = UITableViewCellAccessoryNone;
  [self configNavBar];
}

- (void)configNavBar
{
  [super configNavBar];
  self.title = L(@"editor_report_problem_title");
}

- (void)send
{
  if (!self.isCellSelected)
    return;
  [self sendNote:kAmenityDoesntExist];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  MWMReportBaseController * dvc = segue.destinationViewController;
  NSAssert([dvc isKindOfClass:[MWMReportBaseController class]], @"Incorrect destination controller!");
  dvc.point = self.point;
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

#import "MWMRecentTrackSettingsController.h"
#import "SelectableCell.h"

@interface MWMRecentTrackSettingsController ()
@property (weak, nonatomic) IBOutlet SelectableCell * none;
@property (weak, nonatomic) IBOutlet SelectableCell * oneHour;
@property (weak, nonatomic) IBOutlet SelectableCell * twoHours;
@property (weak, nonatomic) IBOutlet SelectableCell * sixHours;
@property (weak, nonatomic) IBOutlet SelectableCell * twelveHours;
@property (weak, nonatomic) IBOutlet SelectableCell * oneDay;
@property (weak, nonatomic) SelectableCell * selectedCell;

@end

@implementation MWMRecentTrackSettingsController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"recent_track");
  self.none.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell = self.none;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell.accessoryType = UITableViewCellAccessoryNone;
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  self.selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell.selected = NO;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  if (section != 0)
    NSAssert(false, @"Incorrect sections count");
  return L(@"recent_track_help_text");
}

@end

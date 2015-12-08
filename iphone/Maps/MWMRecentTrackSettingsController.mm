#import "MWMRecentTrackSettingsController.h"
#import "SelectableCell.h"

#include "Framework.h"

typedef NS_ENUM(NSUInteger, DurationInHours)
{
  One = 1,
  Two = 2,
  Six = 6,
  Twelve = 12,
  Day = 24
};

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
  auto & f = GetFramework();

  if (!f.IsGpsTrackingEnabled())
  {
    _selectedCell = self.none;
  }
  else
  {
    switch (f.GetGpsTrackingDuration().count())
    {
    case One:
      _selectedCell = self.oneHour;
      break;
    case Two:
      _selectedCell = self.twoHours;
      break;
    case Six:
      _selectedCell = self.sixHours;
      break;
    case Twelve:
      _selectedCell = self.twelveHours;
      break;
    case Day:
      _selectedCell = self.oneDay;
      break;
    default:
      NSAssert(false, @"Incorrect hours value");
      break;
    }
  }
  self.selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
}

- (void)setSelectedCell:(SelectableCell *)selectedCell
{
  _selectedCell = selectedCell;
  auto & f = GetFramework();
  if ([selectedCell isEqual:self.none])
    f.EnableGpsTracking(false);
  else
  {
    if (!f.IsGpsTrackingEnabled())
      f.EnableGpsTracking(true);

    if ([selectedCell isEqual:self.oneHour])
      f.SetGpsTrackingDuration(hours(One));
    else if ([selectedCell isEqual:self.twoHours])
      f.SetGpsTrackingDuration(hours(Two));
    else if ([selectedCell isEqual:self.sixHours])
      f.SetGpsTrackingDuration(hours(Six));
    else if ([selectedCell isEqual:self.twelveHours])
      f.SetGpsTrackingDuration(hours(Twelve));
    else
      f.SetGpsTrackingDuration(hours(Day));
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell.accessoryType = UITableViewCellAccessoryNone;
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  self.selectedCell.selected = NO;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  NSAssert(section == 0, @"Incorrect sections count");
  return L(@"recent_track_help_text");
}

@end

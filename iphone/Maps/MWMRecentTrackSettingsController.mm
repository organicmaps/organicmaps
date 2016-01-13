#import "MWMRecentTrackSettingsController.h"
#import "SelectableCell.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

typedef NS_ENUM(NSUInteger, DurationInHours)
{
  One = 1,
  Two = 2,
  Six = 6,
  Twelve = 12,
  Day = 24,
  TwoDays = 48
};

@interface MWMRecentTrackSettingsController ()

@property (weak, nonatomic) IBOutlet SelectableCell * none;
@property (weak, nonatomic) IBOutlet SelectableCell * oneHour;
@property (weak, nonatomic) IBOutlet SelectableCell * twoHours;
@property (weak, nonatomic) IBOutlet SelectableCell * sixHours;
@property (weak, nonatomic) IBOutlet SelectableCell * twelveHours;
@property (weak, nonatomic) IBOutlet SelectableCell * oneDay;
@property (weak, nonatomic) IBOutlet SelectableCell * twoDays;
@property (weak, nonatomic) SelectableCell * selectedCell;

@end

@implementation MWMRecentTrackSettingsController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_track_record_title");

  if (!GpsTracker::Instance().IsEnabled())
  {
    _selectedCell = self.none;
  }
  else
  {
    switch (GpsTracker::Instance().GetDuration().count())
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
    case TwoDays:
      _selectedCell = self.twoDays;
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
  {
    f.DisconnectFromGpsTracker();
    GpsTracker::Instance().SetEnabled(false);
  }
  else
  {
    GpsTracker::Instance().SetEnabled(true);
    f.ConnectToGpsTracker();

    if ([selectedCell isEqual:self.oneHour])
      GpsTracker::Instance().SetDuration(hours(One));
    else if ([selectedCell isEqual:self.twoHours])
      GpsTracker::Instance().SetDuration(hours(Two));
    else if ([selectedCell isEqual:self.sixHours])
      GpsTracker::Instance().SetDuration(hours(Six));
    else if ([selectedCell isEqual:self.twelveHours])
      GpsTracker::Instance().SetDuration(hours(Twelve));
    else if ([selectedCell isEqual:self.oneDay])
      GpsTracker::Instance().SetDuration(hours(Day));
    else
      GpsTracker::Instance().SetDuration(hours(TwoDays));
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

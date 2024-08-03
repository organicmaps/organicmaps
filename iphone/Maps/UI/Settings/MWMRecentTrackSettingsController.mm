#import "MWMRecentTrackSettingsController.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "map/gps_tracker.hpp"

typedef NS_ENUM(NSUInteger, DurationInHours) { OneHour = 1, TwoHours = 2, SixHours = 6, TwelveHours = 12, OneDay = 24, ThreeDays = 72, OneWeek = 168 };

@interface MWMRecentTrackSettingsController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * none;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * oneHour;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * twoHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * sixHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * twelveHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * oneDay;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * threeDays;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * oneWeek;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

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
    case OneHour: _selectedCell = self.oneHour; break;
    case TwoHours: _selectedCell = self.twoHours; break;
    case SixHours: _selectedCell = self.sixHours; break;
    case TwelveHours: _selectedCell = self.twelveHours; break;
    case OneDay: _selectedCell = self.oneDay; break;
    case ThreeDays: _selectedCell = self.threeDays; break;
    case OneWeek: _selectedCell = self.oneWeek; break;
    default: NSAssert(false, @"Incorrect hours value"); break;
    }
  }
  self.selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
}

- (void)setSelectedCell:(SettingsTableViewSelectableCell *)selectedCell
{
  _selectedCell = selectedCell;
  auto & f = GetFramework();
  auto & tracker = GpsTracker::Instance();
  if ([selectedCell isEqual:self.none])
  {
    f.DisconnectFromGpsTracker();
    tracker.SetEnabled(false);
  }
  else
  {
    if (!tracker.IsEnabled())
    {
      tracker.SetEnabled(true);
      [MWMSettings setTrackWarningAlertShown:NO];
    }
    f.ConnectToGpsTracker();

    if ([selectedCell isEqual:self.oneHour])
      tracker.SetDuration(std::chrono::hours(OneHour));
    else if ([selectedCell isEqual:self.twoHours])
      tracker.SetDuration(std::chrono::hours(TwoHours));
    else if ([selectedCell isEqual:self.sixHours])
      tracker.SetDuration(std::chrono::hours(SixHours));
    else if ([selectedCell isEqual:self.twelveHours])
      tracker.SetDuration(std::chrono::hours(TwelveHours));
    else if ([selectedCell isEqual:self.oneDay])
      tracker.SetDuration(std::chrono::hours(OneDay));
    else if ([selectedCell isEqual:self.threeDays])
      tracker.SetDuration(std::chrono::hours(ThreeDays));
    else
      tracker.SetDuration(std::chrono::hours(OneWeek));
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  SettingsTableViewSelectableCell * selectedCell = self.selectedCell;
  selectedCell.accessoryType = UITableViewCellAccessoryNone;
  selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  selectedCell.selected = NO;
  self.selectedCell = selectedCell;
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  NSAssert(section == 0, @"Incorrect sections count");
  return L(@"recent_track_help_text");
}

@end

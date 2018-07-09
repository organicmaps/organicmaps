#import "MWMRecentTrackSettingsController.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

typedef NS_ENUM(NSUInteger, DurationInHours) { One = 1, Two = 2, Six = 6, Twelve = 12, Day = 24 };

@interface MWMRecentTrackSettingsController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * none;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * oneHour;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * twoHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * sixHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * twelveHours;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * oneDay;
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
    case One: _selectedCell = self.oneHour; break;
    case Two: _selectedCell = self.twoHours; break;
    case Six: _selectedCell = self.sixHours; break;
    case Twelve: _selectedCell = self.twelveHours; break;
    case Day: _selectedCell = self.oneDay; break;
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
  NSString * statValue = nil;
  if ([selectedCell isEqual:self.none])
  {
    f.DisconnectFromGpsTracker();
    tracker.SetEnabled(false);
    statValue = kStatOff;
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
      tracker.SetDuration(hours(One));
    else if ([selectedCell isEqual:self.twoHours])
      tracker.SetDuration(hours(Two));
    else if ([selectedCell isEqual:self.sixHours])
      tracker.SetDuration(hours(Six));
    else if ([selectedCell isEqual:self.twelveHours])
      tracker.SetDuration(hours(Twelve));
    else
      tracker.SetDuration(hours(Day));

    statValue = [NSString stringWithFormat:@"%@ hour(s)", @(tracker.GetDuration().count())];
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  [Statistics logEvent:kStatChangeRecentTrack withParameters:@{kStatValue : statValue}];
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

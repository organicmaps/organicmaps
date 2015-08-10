
#import "SettingsViewController.h"
#import "SwitchCell.h"
#import "SelectableCell.h"
#import "LinkCell.h"
#import "WebViewController.h"
#import "UIKitCategories.h"
#include "../../../platform/settings.hpp"
#include "../../../platform/platform.hpp"
#include "../../../platform/preferred_languages.hpp"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Framework.h"
#import "Statistics.h"
#import "MWMMapViewControlsManager.h"

typedef NS_ENUM(NSUInteger, Section)
{
  SectionMetrics,
  SectionZoomButtons,
  SectionCalibration,
  SectionStatistics,
  SectionCount
};

@interface SettingsViewController () <SwitchCellDelegate>

@end

@implementation SettingsViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"settings");
  self.tableView.backgroundView = nil;
  self.tableView.backgroundColor = [UIColor applicationBackgroundColor];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  GetFramework().Invalidate(true);
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return SectionCount;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == SectionMetrics)
    return 2;
  else
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if (indexPath.section == SectionMetrics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SelectableCell className]];
    Settings::Units units = Settings::Metric;
    (void)Settings::Get("Units", units);
    BOOL selected = units == unitsForIndex(indexPath.row);

    SelectableCell * customCell = (SelectableCell *)cell;
    customCell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    customCell.titleLabel.text = indexPath.row == 0 ? L(@"kilometres") : L(@"miles");
  }
  else if (indexPath.section == SectionStatistics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = true;
    (void)Settings::Get("StatisticsEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"allow_statistics");
    customCell.delegate = self;
  }
  else if (indexPath.section == SectionZoomButtons)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = true;
    (void)Settings::Get("ZoomButtonsEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"pref_zoom_title");
    customCell.delegate = self;
  }
  else if (indexPath.section == SectionCalibration)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = false;
    (void)Settings::Get("CompassCalibrationEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"pref_calibration_title");
    customCell.delegate = self;
  }

  return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  if (section == SectionStatistics)
    return L(@"allow_statistics_hint");
  else if (section == SectionZoomButtons)
    return L(@"pref_zoom_summary");
  return nil;
}

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  if (indexPath.section == SectionStatistics)
  {
    [[Statistics instance] logEvent:@"StatisticsStatusChanged" withParameters:@{@"Enabled" : @(value)}];
    Settings::Set("StatisticsEnabled", (bool)value);
  }
  else if (indexPath.section == SectionZoomButtons)
  {
    Settings::Set("ZoomButtonsEnabled", (bool)value);
    [MapsAppDelegate theApp].m_mapViewController.controlsManager.zoomHidden = !value;
  }
  else if (indexPath.section == SectionCalibration)
  {
    Settings::Set("CompassCalibrationEnabled", (bool)value);
  }
}

Settings::Units unitsForIndex(NSInteger index)
{
  return index == 0 ? Settings::Metric : Settings::Foot;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == SectionMetrics)
  {
    Settings::Units units = unitsForIndex(indexPath.row);
    Settings::Set("Units", units);
    [tableView reloadSections:[NSIndexSet indexSetWithIndex:SectionMetrics] withRowAnimation:UITableViewRowAnimationFade];
    [[MapsAppDelegate theApp].m_mapViewController setupMeasurementSystem];
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == SectionMetrics)
    return L(@"measurement_units");
  else
    return nil;
}

@end

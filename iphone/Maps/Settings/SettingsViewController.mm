#import "Common.h"
#import "LinkCell.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMTextToSpeech.h"
#import "SelectableCell.h"
#import "SettingsViewController.h"
#import "Statistics.h"
#import "SwitchCell.h"
#import "WebViewController.h"

#include "Framework.h"

#include "platform/settings.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

extern char const * kStatisticsEnabledSettingsKey;
char const * kAdForbiddenSettingsKey = "AdForbidden";
extern NSString * const kTTSStatusWasChangedNotification = @"TTFStatusWasChangedFromSettingsNotification";

typedef NS_ENUM(NSUInteger, Section)
{
  SectionMetrics,
  SectionZoomButtons,
  SectionRouting,
  SectionCalibration,
  SectionAd,
  SectionStatistics
};

@interface SettingsViewController () <SwitchCellDelegate>

@end

@implementation SettingsViewController
{
  vector<Section> sections;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"settings");
  self.tableView.backgroundView = nil;
  self.tableView.backgroundColor = [UIColor applicationBackgroundColor];
  if (isIOSVersionLessThan(8))
    sections = {SectionMetrics, SectionZoomButtons, SectionRouting, SectionCalibration, SectionStatistics};
  else
    sections = {SectionMetrics, SectionZoomButtons, SectionRouting, SectionCalibration, SectionAd, SectionStatistics};
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
  return sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (sections[section] == SectionMetrics || sections[section] == SectionRouting)
    return 2;
  else
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  Section section = sections[indexPath.section];
  if (section == SectionMetrics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SelectableCell className]];
    Settings::Units units = Settings::Metric;
    (void)Settings::Get("Units", units);
    BOOL selected = units == unitsForIndex(indexPath.row);

    SelectableCell * customCell = (SelectableCell *)cell;
    customCell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    customCell.titleLabel.text = indexPath.row == 0 ? L(@"kilometres") : L(@"miles");
  }
  else if (section == SectionAd)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool forbidden = false;
    (void)Settings::Get(kAdForbiddenSettingsKey, forbidden);
    customCell.switchButton.on = !forbidden;
    customCell.titleLabel.text = L(@"showcase_settings_title");
    customCell.delegate = self;
  }
  else if (section == SectionStatistics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = [Statistics isStatisticsEnabledByDefault];
    (void)Settings::Get(kStatisticsEnabledSettingsKey, on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"allow_statistics");
    customCell.delegate = self;
  }
  else if (section == SectionZoomButtons)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = true;
    (void)Settings::Get("ZoomButtonsEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"pref_zoom_title");
    customCell.delegate = self;
  }
  else if (section == SectionCalibration)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = false;
    (void)Settings::Get("CompassCalibrationEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"pref_calibration_title");
    customCell.delegate = self;
  }
  else if (section == SectionRouting)
  {
    if (indexPath.row == 0)
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = (SwitchCell *)cell;
      customCell.switchButton.on = [[MWMTextToSpeech tts] isNeedToEnable];
      customCell.titleLabel.text = L(@"pref_tts_enable_title");
      customCell.delegate = self;
    }
    else
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[LinkCell className]];
      LinkCell * customCell = (LinkCell *)cell;
      customCell.titleLabel.text = L(@"pref_tts_language_title");
    }
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
  Statistics * stat = [Statistics instance];
  Section section = sections[indexPath.section];
  if (section == SectionAd)
  {
    [stat logEvent:kStatSettings
        withParameters:@{kStatAction : kStatMoreApps, kStatValue : (value ? kStatOn : kStatOff)}];
    Settings::Set(kAdForbiddenSettingsKey, (bool)!value);
  }
  else if (section == SectionStatistics)
  {
    [stat logEvent:kStatEventName(kStatSettings, kStatToggleStatistics)
        withParameters:
            @{kStatAction : kStatToggleStatistics, kStatValue : (value ? kStatOn : kStatOff)}];
    if (value)
      [stat enableOnNextAppLaunch];
    else
      [stat disableOnNextAppLaunch];
  }
  else if (section == SectionZoomButtons)
  {
    [stat logEvent:kStatEventName(kStatSettings, kStatToggleZoomButtonsVisibility)
        withParameters:@{kStatValue : (value ? kStatVisible : kStatHidden)}];
    Settings::Set("ZoomButtonsEnabled", (bool)value);
    [MapsAppDelegate theApp].mapViewController.controlsManager.zoomHidden = !value;
  }
  else if (section == SectionCalibration)
  {
    [stat logEvent:kStatEventName(kStatSettings, kStatToggleCompassCalibration)
        withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    Settings::Set("CompassCalibrationEnabled", (bool)value);
  }
  else if (section == SectionRouting)
  {
    [[Statistics instance] logEvent:kStatEventName(kStatSettings, kStatTTS)
                     withParameters:@{kStatValue : value ? kStatOn : kStatOff}];
    [[MWMTextToSpeech tts] setNeedToEnable:value];
    [[NSNotificationCenter defaultCenter] postNotificationName:kTTSStatusWasChangedNotification
                                                        object:nil
                                                      userInfo:@{@"on" : @(value)}];
  }
}

Settings::Units unitsForIndex(NSInteger index)
{
  return index == 0 ? Settings::Metric : Settings::Foot;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  Section section = sections[indexPath.section];
  if (section == SectionMetrics)
  {
    Settings::Units units = unitsForIndex(indexPath.row);
    [[Statistics instance] logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
        withParameters:@{kStatValue : (units == Settings::Units::Metric ? kStatKilometers : kStatMiles)}];
    Settings::Set("Units", units);
    [tableView reloadSections:[NSIndexSet indexSetWithIndex:SectionMetrics] withRowAnimation:UITableViewRowAnimationFade];
    [[MapsAppDelegate theApp].mapViewController setupMeasurementSystem];
  }
  else if (section == SectionRouting && indexPath.row == 1)
  {
    [[Statistics instance] logEvent:kStatEventName(kStatSettings, kStatTTS)
                     withParameters:@{kStatAction : kStatChangeLanguage}];
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == SectionMetrics)
    return L(@"measurement_units");
  else if (section == SectionRouting)
    return L(@"prefs_group_route");
  else
    return nil;
}

@end

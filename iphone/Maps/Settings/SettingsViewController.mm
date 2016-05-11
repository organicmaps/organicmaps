#import "Common.h"
#import "LinkCell.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapViewControlsManager.h"
#import "MWMTextToSpeech.h"
#import "SelectableCell.h"
#import "SettingsViewController.h"
#import "Statistics.h"
#import "SwitchCell.h"
#import "UIColor+MapsMeColor.h"
#import "WebViewController.h"

#include "Framework.h"

#include "platform/settings.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

extern char const * kStatisticsEnabledSettingsKey;
char const * kAdForbiddenSettingsKey = "AdForbidden";
char const * kAdServerForbiddenKey = "AdServerForbidden";
char const * kAutoDownloadEnabledKey = "AutoDownloadEnabled";
extern NSString * const kTTSStatusWasChangedNotification = @"TTFStatusWasChangedFromSettingsNotification";

typedef NS_ENUM(NSUInteger, Section)
{
  SectionMetrics,
  SectionMap,
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
  bool adServerForbidden = false;
  (void)settings::Get(kAdServerForbiddenKey, adServerForbidden);
  if (isIOS7 || adServerForbidden)
    sections = {SectionMetrics, SectionMap, SectionRouting, SectionCalibration, SectionStatistics};
  else
    sections = {SectionMetrics, SectionMap, SectionRouting, SectionCalibration, SectionAd, SectionStatistics};
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  switch (sections[section])
  {
  case SectionAd:
  case SectionStatistics:
  case SectionCalibration:
    return 1;
  case SectionMetrics:
    return 2;
  case SectionRouting:
    return 3;
  case SectionMap:
    return 5;
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  switch (sections[indexPath.section])
  {
  case SectionMetrics:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SelectableCell className]];
    settings::Units units = settings::Metric;
    (void)settings::Get(settings::kMeasurementUnits, units);
    BOOL const selected = units == unitsForIndex(indexPath.row);
    SelectableCell * customCell = (SelectableCell *)cell;
    customCell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    customCell.titleLabel.text = indexPath.row == 0 ? L(@"kilometres") : L(@"miles");
    break;
  }
  case SectionAd:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool forbidden = false;
    (void)settings::Get(kAdForbiddenSettingsKey, forbidden);
    customCell.switchButton.on = !forbidden;
    customCell.titleLabel.text = L(@"showcase_settings_title");
    customCell.delegate = self;
    break;
  }
  case SectionStatistics:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = [Statistics isStatisticsEnabledByDefault];
    (void)settings::Get(kStatisticsEnabledSettingsKey, on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"allow_statistics");
    customCell.delegate = self;
    break;
  }
  case SectionMap:
  {
    switch (indexPath.row)
    {
    // Night mode
    // Recent track
    case 0:
    case 1:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[LinkCell className]];
      LinkCell * customCell = static_cast<LinkCell *>(cell);
      customCell.titleLabel.text = indexPath.row == 0 ? L(@"pref_map_style_title") : L(@"pref_track_record_title");
      break;
    }
    // Auto download
    case 2:
    {
      bool autoDownloadEnabled = true;
      (void)settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled);
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = static_cast<SwitchCell *>(cell);
      customCell.titleLabel.text = L(@"autodownload");
      customCell.switchButton.on = autoDownloadEnabled;
      customCell.delegate = self;
      break;
    }
    // 3D buildings
    case 3:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = static_cast<SwitchCell *>(cell);
      bool on = true, _ = true;
      GetFramework().Load3dMode(_, on);
      customCell.titleLabel.text = L(@"pref_map_3d_buildings_title");
      customCell.switchButton.on = on;
      customCell.delegate = self;
      break;
    }
    // Zoom buttons
    case 4:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = static_cast<SwitchCell *>(cell);
      bool on = true;
      (void)settings::Get("ZoomButtonsEnabled", on);
      customCell.titleLabel.text = L(@"pref_zoom_title");
      customCell.switchButton.on = on;
      customCell.delegate = self;
      break;
    }
  }
  break;
  }
  case SectionRouting:
  {
    switch (indexPath.row)
    {
    // 3D mode
    case 0:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = (SwitchCell *)cell;
      customCell.titleLabel.text = L(@"pref_map_3d_title");
      customCell.delegate = self;
      bool _ = true, on = true;
      GetFramework().Load3dMode(on, _);
      customCell.switchButton.on = on;
      break;
    }
    // Enable TTS
    case 1:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
      SwitchCell * customCell = (SwitchCell *)cell;
      customCell.switchButton.on = [[MWMTextToSpeech tts] isNeedToEnable];
      customCell.titleLabel.text = L(@"pref_tts_enable_title");
      customCell.delegate = self;
      break;
    }
    // Change TTS language
    case 2:
    {
      cell = [tableView dequeueReusableCellWithIdentifier:[LinkCell className]];
      LinkCell * customCell = (LinkCell *)cell;
      customCell.titleLabel.text = L(@"pref_tts_language_title");
      break;
    }
    default:
      NSAssert(false, @"Incorrect index path!");
      break;
    }
    break;
  }
  case SectionCalibration:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:[SwitchCell className]];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on = false;
    (void)settings::Get("CompassCalibrationEnabled", on);
    customCell.switchButton.on = on;
    customCell.titleLabel.text = L(@"pref_calibration_title");
    customCell.delegate = self;
    break;
  }
  }
  return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  if (section == SectionStatistics)
    return L(@"allow_statistics_hint");
  return nil;
}

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  Statistics * stat = [Statistics instance];
  switch (sections[indexPath.section])
  {
  case SectionAd:
    [Statistics logEvent:kStatSettings
      withParameters:@{kStatAction : kStatMoreApps, kStatValue : (value ? kStatOn : kStatOff)}];
    settings::Set(kAdForbiddenSettingsKey, (bool)!value);
    break;

  case SectionStatistics:
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleStatistics)
        withParameters: @{kStatAction : kStatToggleStatistics, kStatValue : (value ? kStatOn : kStatOff)}];
    if (value)
      [stat enableOnNextAppLaunch];
    else
      [stat disableOnNextAppLaunch];
    break;

  case SectionMap:
    switch (indexPath.row)
    {
      // Night mode
      // Recent track
      case 0:
      case 1:
        break;
      // Auto download
      case 2:
      {
        [Statistics logEvent:kStatEventName(kStatSettings, kStatAutoDownload)
              withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
        settings::Set(kAutoDownloadEnabledKey, (bool)value);
        break;
      }
      // 3D buildings
      case 3:
      {
        [Statistics logEvent:kStatEventName(kStatSettings, kStat3DBuildings)
                         withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
        auto & f = GetFramework();
        bool _ = true, is3dBuildings = true;
        f.Load3dMode(_, is3dBuildings);
        is3dBuildings = static_cast<bool>(value);
        f.Save3dMode(_, is3dBuildings);
        f.Allow3dMode(_, is3dBuildings);
        break;
      }
      // Zoom buttons
      case 4:
      {
        [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleZoomButtonsVisibility)
            withParameters:@{kStatValue : (value ? kStatVisible : kStatHidden)}];
        settings::Set("ZoomButtonsEnabled", (bool)value);
        [MapsAppDelegate theApp].mapViewController.controlsManager.zoomHidden = !value;
        break;
      }
    }
    break;

  case SectionCalibration:
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleCompassCalibration)
        withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    settings::Set("CompassCalibrationEnabled", (bool)value);
    break;

  case SectionRouting:
    // 3D mode
    if (indexPath.row == 0)
    {
      [Statistics logEvent:kStatEventName(kStatSettings, kStat3D)
                       withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
      auto & f = GetFramework();
      bool _ = true, is3d = true;
      f.Load3dMode(is3d, _);
      is3d = static_cast<bool>(value);
      f.Save3dMode(is3d, _);
      f.Allow3dMode(is3d, _);
    }
    // Enable TTS
    else if (indexPath.row == 1)
    {
      [Statistics logEvent:kStatEventName(kStatSettings, kStatTTS)
                       withParameters:@{kStatValue : value ? kStatOn : kStatOff}];
      [[MWMTextToSpeech tts] setNeedToEnable:value];
      [[NSNotificationCenter defaultCenter] postNotificationName:kTTSStatusWasChangedNotification
                                                          object:nil userInfo:@{@"on" : @(value)}];
    }
    break;

  case SectionMetrics:
    break;
  }
}

settings::Units unitsForIndex(NSInteger index)
{
  return index == 0 ? settings::Metric : settings::Foot;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch (sections[indexPath.section])
  {
  case SectionMetrics:
  {
    settings::Units units = unitsForIndex(indexPath.row);
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
        withParameters:@{kStatValue : (units == settings::Units::Metric ? kStatKilometers : kStatMiles)}];
    settings::Set(settings::kMeasurementUnits, units);
    [tableView reloadSections:[NSIndexSet indexSetWithIndex:SectionMetrics] withRowAnimation:UITableViewRowAnimationFade];
    GetFramework().SetupMeasurementSystem();
    break;
  }
  case SectionRouting:
    // Change TTS language
    if (indexPath.row == 2)
    {
      [Statistics logEvent:kStatEventName(kStatSettings, kStatTTS)
                     withParameters:@{kStatAction : kStatChangeLanguage}];
      [self performSegueWithIdentifier:@"SettingsToTTSSegue" sender:nil];
    }
    break;
  case SectionMap:
    // Change night mode
    if (indexPath.row == 0)
    {
      [Statistics logEvent:kStatEventName(kStatSettings, kStatNightMode)
                       withParameters:@{kStatAction : kStatChangeNightMode}];
      [self performSegueWithIdentifier:@"SettingsToNightMode" sender:nil];
    }
    else if (indexPath.row == 1)
    {
      [Statistics logEvent:kStatEventName(kStatSettings, kStatRecentTrack)
                       withParameters:@{kStatAction : kStatChangeRecentTrack}];
      [self performSegueWithIdentifier:@"SettingsToRecentTrackSegue" sender:nil];
    }
    break;
  case SectionAd:
  case SectionCalibration:
  case SectionStatistics:
    break;
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (sections[section])
  {
  case SectionMetrics:
    return L(@"measurement_units");
  case SectionRouting:
    return L(@"prefs_group_route");
  case SectionMap:
    return L(@"prefs_group_map");
  case SectionCalibration:
  case SectionAd:
  case SectionStatistics:
    return nil;
  }
}

@end

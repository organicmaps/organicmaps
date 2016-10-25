#import "MWMSettingsViewController.h"
#import "LinkCell.h"
#import "LocaleTranslator.h"
#import "MWMAuthorizationCommon.h"
#import "MWMSettings.h"
#import "MWMTextToSpeech.h"
#import "Statistics.h"
#import "SwitchCell.h"
#import "WebViewController.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSettingsViewController ()<SwitchCellDelegate>

@property(weak, nonatomic) IBOutlet LinkCell * profileCell;

@property(weak, nonatomic) IBOutlet LinkCell * unitsCell;
@property(weak, nonatomic) IBOutlet SwitchCell * zoomButtonsCell;
@property(weak, nonatomic) IBOutlet SwitchCell * is3dCell;
@property(weak, nonatomic) IBOutlet SwitchCell * autoDownloadCell;
@property(weak, nonatomic) IBOutlet LinkCell * recentTrackCell;
@property(weak, nonatomic) IBOutlet SwitchCell * compassCalibrationCell;
@property(weak, nonatomic) IBOutlet SwitchCell * statisticsCell;

@property(weak, nonatomic) IBOutlet LinkCell * nightModeCell;
@property(weak, nonatomic) IBOutlet SwitchCell * perspectiveViewCell;
@property(weak, nonatomic) IBOutlet SwitchCell * autoZoomCell;
@property(weak, nonatomic) IBOutlet LinkCell * voiceInstructionsCell;

@property(weak, nonatomic) IBOutlet LinkCell * helpCell;
@property(weak, nonatomic) IBOutlet LinkCell * aboutCell;

@end

@implementation MWMSettingsViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"settings");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self configCells];
}

- (void)configCells
{
  [self configProfileSection];
  [self configCommonSection];
  [self configNavigationSection];
}

- (void)configProfileSection
{
  NSString * userName = osm_auth_ios::OSMUserName();
  self.profileCell.infoLabel.text = userName.length != 0 ? userName : @"";
}

- (void)configCommonSection
{
  switch ([MWMSettings measurementUnits])
  {
  case measurement_utils::Units::Metric: self.unitsCell.infoLabel.text = L(@"kilometres"); break;
  case measurement_utils::Units::Imperial: self.unitsCell.infoLabel.text = L(@"miles"); break;
  }

  self.zoomButtonsCell.switchButton.on = [MWMSettings zoomButtonsEnabled];
  self.zoomButtonsCell.delegate = self;

  bool on = true, _ = true;
  GetFramework().Load3dMode(_, on);
  self.is3dCell.switchButton.on = on;
  self.is3dCell.delegate = self;

  self.autoDownloadCell.switchButton.on = [MWMSettings autoDownloadEnabled];
  self.autoDownloadCell.delegate = self;

  if (!GpsTracker::Instance().IsEnabled())
  {
    self.recentTrackCell.infoLabel.text = L(@"duration_disabled");
  }
  else
  {
    switch (GpsTracker::Instance().GetDuration().count())
    {
    case 1: self.recentTrackCell.infoLabel.text = L(@"duration_1_hour"); break;
    case 2: self.recentTrackCell.infoLabel.text = L(@"duration_2_hours"); break;
    case 6: self.recentTrackCell.infoLabel.text = L(@"duration_6_hours"); break;
    case 12: self.recentTrackCell.infoLabel.text = L(@"duration_12_hours"); break;
    case 24: self.recentTrackCell.infoLabel.text = L(@"duration_1_day"); break;
    default: NSAssert(false, @"Incorrect hours value"); break;
    }
  }

  self.compassCalibrationCell.switchButton.on = [MWMSettings compassCalibrationEnabled];
  self.compassCalibrationCell.delegate = self;

  self.statisticsCell.switchButton.on = [MWMSettings statisticsEnabled];
  self.statisticsCell.delegate = self;
}

- (void)configNavigationSection
{
  if ([MWMSettings autoNightModeEnabled])
  {
    self.nightModeCell.infoLabel.text = L(@"pref_map_style_auto");
  }
  else
  {
    switch (GetFramework().GetMapStyle())
    {
    case MapStyleDark: self.nightModeCell.infoLabel.text = L(@"pref_map_style_night"); break;
    default: self.nightModeCell.infoLabel.text = L(@"pref_map_style_default"); break;
    }
  }

  bool _ = true, on = true;
  GetFramework().Load3dMode(on, _);
  self.perspectiveViewCell.switchButton.on = on;
  self.perspectiveViewCell.delegate = self;

  self.autoZoomCell.switchButton.on = GetFramework().LoadAutoZoom();
  self.autoZoomCell.delegate = self;

  if ([MWMTextToSpeech isTTSEnabled])
  {
    NSString * savedLanguage = [MWMTextToSpeech savedLanguage];
    if (savedLanguage.length != 0)
    {
      string const savedLanguageTwine = locale_translator::bcp47ToTwineLanguage(savedLanguage);
      NSString * language = @(tts::translatedTwine(savedLanguageTwine).c_str());
      self.voiceInstructionsCell.infoLabel.text = language;
    }
    else
    {
      self.voiceInstructionsCell.infoLabel.text = @"";
    }
  }
  else
  {
    self.voiceInstructionsCell.infoLabel.text = L(@"duration_disabled");
  }
}

#pragma mark - SwitchCellDelegate

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value
{
  if (cell == self.zoomButtonsCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleZoomButtonsVisibility)
          withParameters:@{kStatValue : (value ? kStatVisible : kStatHidden)}];
    [MWMSettings setZoomButtonsEnabled:value];
  }
  else if (cell == self.is3dCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStat3DBuildings)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    auto & f = GetFramework();
    bool _ = true, is3dBuildings = true;
    f.Load3dMode(_, is3dBuildings);
    is3dBuildings = static_cast<bool>(value);
    f.Save3dMode(_, is3dBuildings);
    f.Allow3dMode(_, is3dBuildings);
  }
  else if (cell == self.autoDownloadCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatAutoDownload)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setAutoDownloadEnabled:value];
  }
  else if (cell == self.compassCalibrationCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleCompassCalibration)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setCompassCalibrationEnabled:value];
  }
  else if (cell == self.statisticsCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleStatistics)
          withParameters:@{
            kStatAction : kStatToggleStatistics,
            kStatValue : (value ? kStatOn : kStatOff)
          }];
    [MWMSettings setStatisticsEnabled:value];
  }
  else if (cell == self.perspectiveViewCell)
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
  else if (cell == self.autoZoomCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatAutoZoom)
          withParameters:@{kStatValue : value ? kStatOn : kStatOff}];
    auto & f = GetFramework();
    f.AllowAutoZoom(value);
    f.SaveAutoZoom(value);
  }
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  LinkCell * cell = static_cast<LinkCell *>([tableView cellForRowAtIndexPath:indexPath]);
  if (cell == self.profileCell)
  {
    [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatAuthorization}];
    [self performSegueWithIdentifier:@"SettingsToProfileSegue" sender:nil];
  }
  else if (cell == self.unitsCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
          withParameters:@{kStatAction : kStatChangeMeasureUnits}];
    [self performSegueWithIdentifier:@"SettingsToUnits" sender:nil];
  }
  else if (cell == self.recentTrackCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatRecentTrack)
          withParameters:@{kStatAction : kStatChangeRecentTrack}];
    [self performSegueWithIdentifier:@"SettingsToRecentTrackSegue" sender:nil];
  }
  else if (cell == self.nightModeCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatNightMode)
          withParameters:@{kStatAction : kStatChangeNightMode}];
    [self performSegueWithIdentifier:@"SettingsToNightMode" sender:nil];
  }
  else if (cell == self.voiceInstructionsCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatTTS)
          withParameters:@{kStatAction : kStatChangeLanguage}];
    [self performSegueWithIdentifier:@"SettingsToTTSSegue" sender:nil];
  }
  else if (cell == self.helpCell)
  {
    [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatHelp}];
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"help"];
    [self performSegueWithIdentifier:@"SettingsToHelp" sender:nil];
  }
  else if (cell == self.aboutCell)
  {
    [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatAbout}];
    [self performSegueWithIdentifier:@"SettingsToAbout" sender:nil];
  }
}

#pragma mark - UITableViewDataSource

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (section)
  {
  case 1: return L(@"general_settings");
  case 2: return L(@"prefs_group_route");
  case 3: return L(@"info");
  default: return nil;
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  switch (section)
  {
  case 1: return L(@"allow_statistics_hint");
  default: return nil;
  }
}

@end

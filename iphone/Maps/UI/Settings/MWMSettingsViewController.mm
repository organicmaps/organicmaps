#import "MWMSettingsViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMNetworkPolicy.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

#include "LocaleTranslator.h"

#include "Framework.h"

#include "map/gps_tracker.hpp"

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSettingsViewController ()<SettingsTableViewSwitchCellDelegate>

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * profileCell;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * unitsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * zoomButtonsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * is3dCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * autoDownloadCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * mobileInternetCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * recentTrackCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * fontScaleCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * transliterationCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * compassCalibrationCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * showOffersCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * statisticsCell;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * nightModeCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * perspectiveViewCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * autoZoomCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * voiceInstructionsCell;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * helpCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * aboutCell;

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
  [self configInfoSection];
}

- (void)configProfileSection
{
  NSString * userName = osm_auth_ios::OSMUserName();
  [self.profileCell configWithTitle:L(@"profile") info:userName.length != 0 ? userName : @""];
}

- (void)configCommonSection
{
  NSString * units = nil;
  switch ([MWMSettings measurementUnits])
  {
  case MWMUnitsMetric: units = L(@"kilometres"); break;
  case MWMUnitsImperial: units = L(@"miles"); break;
  }
  [self.unitsCell configWithTitle:L(@"measurement_units") info:units];

  [self.zoomButtonsCell configWithDelegate:self
                                     title:L(@"pref_zoom_title")
                                      isOn:[MWMSettings zoomButtonsEnabled]];

  bool on = true, _ = true;
  GetFramework().Load3dMode(_, on);
  [self.is3dCell configWithDelegate:self title:L(@"pref_map_3d_buildings_title") isOn:on];

  [self.autoDownloadCell configWithDelegate:self
                                      title:L(@"disable_autodownload")
                                       isOn:![MWMSettings autoDownloadEnabled]];

  NSString * mobileInternet = nil;
  using stage = platform::NetworkPolicy::Stage;
  switch (network_policy::GetStage())
  {
  case stage::Always: mobileInternet = L(@"mobile_data_option_always"); break;
  case stage::Session: mobileInternet = L(@"mobile_data_option_today"); break;
  case stage::Never: mobileInternet = L(@"mobile_data_option_never"); break;
  }
  [self.mobileInternetCell configWithTitle:L(@"mobile_data") info:mobileInternet];


  NSString * recentTrack = nil;
  if (!GpsTracker::Instance().IsEnabled())
  {
    recentTrack = L(@"duration_disabled");
  }
  else
  {
    switch (GpsTracker::Instance().GetDuration().count())
    {
    case 1: recentTrack = L(@"duration_1_hour"); break;
    case 2: recentTrack = L(@"duration_2_hours"); break;
    case 6: recentTrack = L(@"duration_6_hours"); break;
    case 12: recentTrack = L(@"duration_12_hours"); break;
    case 24: recentTrack = L(@"duration_1_day"); break;
    default: NSAssert(false, @"Incorrect hours value"); break;
    }
  }
  [self.recentTrackCell configWithTitle:L(@"pref_track_record_title") info:recentTrack];

  [self.fontScaleCell configWithDelegate:self
                                   title:L(@"big_font")
                                    isOn:[MWMSettings largeFontSize]];

  [self.transliterationCell configWithDelegate:self
                                         title:L(@"whatsnew_transliteration_title")
                                          isOn:[MWMSettings transliteration]];

  [self.compassCalibrationCell configWithDelegate:self
                                            title:L(@"pref_calibration_title")
                                             isOn:[MWMSettings compassCalibrationEnabled]];

  [self.showOffersCell configWithDelegate:self
                                    title:L(@"showcase_settings_title")
                                     isOn:![MWMSettings adForbidden]];

  [self.statisticsCell configWithDelegate:self
                                    title:L(@"allow_statistics")
                                     isOn:[MWMSettings statisticsEnabled]];
}

- (void)configNavigationSection
{
  NSString * nightMode = nil;
  switch ([MWMSettings theme])
  {
  case MWMThemeVehicleDay: NSAssert(false, @"Invalid case");
  case MWMThemeDay: nightMode = L(@"pref_map_style_default"); break;
  case MWMThemeVehicleNight: NSAssert(false, @"Invalid case");
  case MWMThemeNight: nightMode = L(@"pref_map_style_night"); break;
  case MWMThemeAuto: nightMode = L(@"pref_map_style_auto"); break;
  }
  [self.nightModeCell configWithTitle:L(@"pref_map_style_title") info:nightMode];

  bool _ = true, on = true;
  GetFramework().Load3dMode(on, _);
  [self.perspectiveViewCell configWithDelegate:self title:L(@"pref_map_3d_title") isOn:on];

  [self.autoZoomCell configWithDelegate:self
                                  title:L(@"pref_map_auto_zoom")
                                   isOn:GetFramework().LoadAutoZoom()];

  NSString * voiceInstructions = nil;
  if ([MWMTextToSpeech isTTSEnabled])
  {
    NSString * savedLanguage = [MWMTextToSpeech savedLanguage];
    if (savedLanguage.length != 0)
    {
      string const savedLanguageTwine = locale_translator::bcp47ToTwineLanguage(savedLanguage);
      voiceInstructions = @(tts::translatedTwine(savedLanguageTwine).c_str());
    }
  }
  else
  {
    voiceInstructions = L(@"duration_disabled");
  }
  [self.voiceInstructionsCell configWithTitle:L(@"pref_tts_language_title") info:voiceInstructions];
}

- (void)configInfoSection
{
  [self.helpCell configWithTitle:L(@"help") info:nil];
  [self.aboutCell configWithTitle:L(@"about_menu_title") info:nil];
}

#pragma mark - SettingsTableViewSwitchCellDelegate

- (void)switchCell:(SettingsTableViewSwitchCell *)cell didChangeValue:(BOOL)value
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
    [MWMSettings setAutoDownloadEnabled:!value];
  }
  else if (cell == self.fontScaleCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleLargeFontSize)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setLargeFontSize:value];
  }
  else if (cell == self.transliterationCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleTransliteration)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setTransliteration:value];
  }
  else if (cell == self.compassCalibrationCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatToggleCompassCalibration)
          withParameters:@{kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setCompassCalibrationEnabled:value];
  }
  else if (cell == self.showOffersCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatAd)
          withParameters:@{kStatAction : kStatAd, kStatValue : (value ? kStatOn : kStatOff)}];
    [MWMSettings setAdForbidden:!value];
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
  auto cell = static_cast<SettingsTableViewLinkCell *>([tableView cellForRowAtIndexPath:indexPath]);
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
  else if (cell == self.mobileInternetCell)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatMobileInternet)
          withParameters:@{kStatAction : kStatChangeMobileInternet}];
    [self performSegueWithIdentifier:@"SettingsToMobileInternetSegue" sender:nil];
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

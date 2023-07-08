#import "MWMSettingsViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

#import <CoreApi/CoreApi.h>

#include "map/gps_tracker.hpp"

using namespace power_management;

@interface MWMSettingsViewController ()<SettingsTableViewSwitchCellDelegate>

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *profileCell;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *unitsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *zoomButtonsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *is3dCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *autoDownloadCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *mobileInternetCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *powerManagementCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *recentTrackCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *fontScaleCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *transliterationCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *compassCalibrationCell;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *nightModeCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *perspectiveViewCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell *autoZoomCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *voiceInstructionsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell *drivingOptionsCell;


@end

@implementation MWMSettingsViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.title = L(@"settings");
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self configCells];
}

- (void)configCells {
  [self configProfileSection];
  [self configCommonSection];
  [self configNavigationSection];
}

- (void)configProfileSection {
  NSString *userName = osm_auth_ios::OSMUserName();
  [self.profileCell configWithTitle:L(@"profile") info:userName.length != 0 ? userName : @""];
}

- (void)configCommonSection {
  NSString *units = nil;
  switch ([MWMSettings measurementUnits]) {
    case MWMUnitsMetric:
      units = L(@"kilometres");
      break;
    case MWMUnitsImperial:
      units = L(@"miles");
      break;
  }
  [self.unitsCell configWithTitle:L(@"measurement_units") info:units];

  [self.zoomButtonsCell configWithDelegate:self title:L(@"pref_zoom_title") isOn:[MWMSettings zoomButtonsEnabled]];

  bool on = true, _ = true;
  GetFramework().Load3dMode(_, on);
  if (GetFramework().GetPowerManager().GetScheme() == Scheme::EconomyMaximum)
  {
    self.is3dCell.isEnabled = false;
    [self.is3dCell configWithDelegate:self title:L(@"pref_map_3d_buildings_title") isOn:false];
    UITapGestureRecognizer* tapRecogniser = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                            action:@selector(show3dBuildingsAlert:)];

    self.is3dCell.gestureRecognizers = @[tapRecogniser];
  }
  else
  {
    self.is3dCell.isEnabled = true;
    [self.is3dCell configWithDelegate:self title:L(@"pref_map_3d_buildings_title") isOn:on];
    self.is3dCell.gestureRecognizers = nil;
  }

  [self.autoDownloadCell configWithDelegate:self
                                      title:L(@"autodownload")
                                       isOn:[MWMSettings autoDownloadEnabled]];

  NSString *mobileInternet = nil;
  switch ([MWMNetworkPolicy sharedPolicy].permission) {
    case MWMNetworkPolicyPermissionAlways:
      mobileInternet = L(@"mobile_data_option_always");
      break;
    case MWMNetworkPolicyPermissionNever:
      mobileInternet = L(@"mobile_data_option_never");
      break;
    case MWMNetworkPolicyPermissionToday:
    case MWMNetworkPolicyPermissionNotToday:
    case MWMNetworkPolicyPermissionAsk:
      mobileInternet = L(@"mobile_data_option_ask");
      break;
  }
  [self.mobileInternetCell configWithTitle:L(@"mobile_data") info:mobileInternet];

  NSString *powerManagement = nil;
  switch (GetFramework().GetPowerManager().GetScheme()) {
    case Scheme::None:
      break;
    case Scheme::Normal:
      powerManagement = L(@"power_managment_setting_never");
      break;
    case Scheme::EconomyMedium:
      break;
    case Scheme::EconomyMaximum:
      powerManagement = L(@"power_managment_setting_manual_max");
      break;
    case Scheme::Auto:
      powerManagement = L(@"power_managment_setting_auto");
      break;
  }
  [self.powerManagementCell configWithTitle:L(@"power_managment_title") info:powerManagement];

  NSString *recentTrack = nil;
  if (!GpsTracker::Instance().IsEnabled()) {
    recentTrack = L(@"duration_disabled");
  } else {
    switch (GpsTracker::Instance().GetDuration().count()) {
      case 1:
        recentTrack = L(@"duration_1_hour");
        break;
      case 2:
        recentTrack = L(@"duration_2_hours");
        break;
      case 6:
        recentTrack = L(@"duration_6_hours");
        break;
      case 12:
        recentTrack = L(@"duration_12_hours");
        break;
      case 24:
        recentTrack = L(@"duration_1_day");
        break;
      default:
        NSAssert(false, @"Incorrect hours value");
        break;
    }
  }
  [self.recentTrackCell configWithTitle:L(@"pref_track_record_title") info:recentTrack];

  [self.fontScaleCell configWithDelegate:self title:L(@"big_font") isOn:[MWMSettings largeFontSize]];

  [self.transliterationCell configWithDelegate:self
                                         title:L(@"transliteration_title")
                                          isOn:[MWMSettings transliteration]];

  [self.compassCalibrationCell configWithDelegate:self
                                            title:L(@"pref_calibration_title")
                                             isOn:[MWMSettings compassCalibrationEnabled]];
}

- (void)show3dBuildingsAlert:(UITapGestureRecognizer *)recognizer {
  UIAlertController *alert =
  [UIAlertController alertControllerWithTitle:L(@"pref_map_3d_buildings_title")
                                      message:L(@"pref_map_3d_buildings_disabled_summary")
                               preferredStyle:UIAlertControllerStyleAlert];

  UIAlertAction *okButton = [UIAlertAction actionWithTitle:@"OK"
                                                      style:UIAlertActionStyleDefault
                                                    handler:nil];
  [alert addAction:okButton];

  [self presentViewController:alert animated:YES completion:nil];
}

- (void)configNavigationSection {
  NSString *nightMode = nil;
  switch ([MWMSettings theme]) {
    case MWMThemeVehicleDay:
      NSAssert(false, @"Invalid case");
    case MWMThemeDay:
      nightMode = L(@"pref_map_style_default");
      break;
    case MWMThemeVehicleNight:
      NSAssert(false, @"Invalid case");
    case MWMThemeNight:
      nightMode = L(@"pref_map_style_night");
      break;
    case MWMThemeAuto:
      nightMode = L(@"pref_map_style_auto");
      break;
  }
  [self.nightModeCell configWithTitle:L(@"pref_map_style_title") info:nightMode];

  bool _ = true, on = true;
  auto &f = GetFramework();
  f.Load3dMode(on, _);
  [self.perspectiveViewCell configWithDelegate:self title:L(@"pref_map_3d_title") isOn:on];

  [self.autoZoomCell configWithDelegate:self title:L(@"pref_map_auto_zoom") isOn:GetFramework().LoadAutoZoom()];

  NSString *ttsEnabledString = [MWMTextToSpeech isTTSEnabled] ? L(@"on") : L(@"off");
  [self.voiceInstructionsCell configWithTitle:L(@"pref_tts_enable_title") info:ttsEnabledString];
  [self.drivingOptionsCell configWithTitle:L(@"driving_options_title") info:@""];
}


#pragma mark - SettingsTableViewSwitchCellDelegate

- (void)switchCell:(SettingsTableViewSwitchCell *)cell didChangeValue:(BOOL)value {
  if (cell == self.zoomButtonsCell) {
    [MWMSettings setZoomButtonsEnabled:value];
  } else if (cell == self.is3dCell) {
    auto &f = GetFramework();
    bool _ = true, is3dBuildings = true;
    f.Load3dMode(_, is3dBuildings);
    is3dBuildings = static_cast<bool>(value);
    f.Save3dMode(_, is3dBuildings);
    f.Allow3dMode(_, is3dBuildings);
  } else if (cell == self.autoDownloadCell) {
    [MWMSettings setAutoDownloadEnabled:value];
  } else if (cell == self.fontScaleCell) {
    [MWMSettings setLargeFontSize:value];
  } else if (cell == self.transliterationCell) {
    [MWMSettings setTransliteration:value];
  } else if (cell == self.compassCalibrationCell) {
    [MWMSettings setCompassCalibrationEnabled:value];
  } else if (cell == self.perspectiveViewCell) {
    auto &f = GetFramework();
    bool _ = true, is3d = true;
    f.Load3dMode(is3d, _);
    is3d = static_cast<bool>(value);
    f.Save3dMode(is3d, _);
    f.Allow3dMode(is3d, _);
  } else if (cell == self.autoZoomCell) {
    auto &f = GetFramework();
    f.AllowAutoZoom(value);
    f.SaveAutoZoom(value);
  }
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  auto cell = [tableView cellForRowAtIndexPath:indexPath];
  if (cell == self.profileCell) {
    [self performSegueWithIdentifier:@"SettingsToProfileSegue" sender:nil];
  } else if (cell == self.unitsCell) {
    [self performSegueWithIdentifier:@"SettingsToUnits" sender:nil];
  } else if (cell == self.mobileInternetCell) {
    [self performSegueWithIdentifier:@"SettingsToMobileInternetSegue" sender:nil];
  } else if (cell == self.powerManagementCell) {
    [self performSegueWithIdentifier:@"SettingsToPowerManagementSegue" sender:nil];
  } else if (cell == self.recentTrackCell) {
    [self performSegueWithIdentifier:@"SettingsToRecentTrackSegue" sender:nil];
  } else if (cell == self.nightModeCell) {
    [self performSegueWithIdentifier:@"SettingsToNightMode" sender:nil];
  } else if (cell == self.voiceInstructionsCell) {
    [self performSegueWithIdentifier:@"SettingsToTTSSegue" sender:nil];
  } else if (cell == self.drivingOptionsCell) {
    [self performSegueWithIdentifier:@"settingsToDrivingOptionsSegue" sender:nil];
  }
}

#pragma mark - UITableViewDataSource

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
  switch (section) {
    case 1:
      return L(@"general_settings");
    case 2:
      return L(@"prefs_group_route");
    case 3:
      return L(@"info");
    default:
      return nil;
  }
}

@end

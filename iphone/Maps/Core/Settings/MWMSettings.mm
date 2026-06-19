#import "MWMSettings.h"
#import "MWMAuthorizationCommon.h"
#import "MWMCoreUnits.h"
#import "MWMMapViewControlsManager.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>
#include <CoreApi/Logger.h>

#include "map/gps_tracker.hpp"

namespace
{
char const * kAutoDownloadEnabledKey = "AutoDownloadEnabled";
char const * kZoomButtonsEnabledKey = "ZoomButtonsEnabled";
char const * kCompassCalibrationEnabledKey = "CompassCalibrationEnabled";
char const * kRoutingDisclaimerApprovedKey = "IsDisclaimerApproved";

// TODO(igrechuhin): Remove outdated kUDAutoNightModeOff
NSString * const kUDAutoNightModeOff = @"AutoNightModeOff";
NSString * const kThemeMode = @"ThemeMode";
NSString * const kSpotlightLocaleLanguageId = @"SpotlightLocaleLanguageId";
NSString * const kUDTrackWarningAlertWasShown = @"TrackWarningAlertWasShown";
NSString * const kiCLoudSynchronizationEnabledKey = @"iCLoudSynchronizationEnabled";
NSString * const kUDFileLoggingEnabledKey = @"FileLoggingEnabledKey";
NSString * const kUDDidShowICloudSynchronizationEnablingAlert = @"kUDDidShowICloudSynchronizationEnablingAlert";
}  // namespace

@implementation MWMSettings

+ (NSString *)osmUserName
{
  return osm_auth_ios::OSMUserName();
}

+ (BOOL)autoDownloadEnabled
{
  bool autoDownloadEnabled = true;
  UNUSED_VALUE(settings::Get(kAutoDownloadEnabledKey, autoDownloadEnabled));
  return autoDownloadEnabled;
}

+ (void)setAutoDownloadEnabled:(BOOL)autoDownloadEnabled
{
  settings::Set(kAutoDownloadEnabledKey, static_cast<bool>(autoDownloadEnabled));
}

+ (MWMUnits)measurementUnits
{
  auto units = measurement_utils::Units::Metric;
  UNUSED_VALUE(settings::Get(settings::kMeasurementUnits, units));
  return mwmUnits(units);
}

+ (void)setMeasurementUnits:(MWMUnits)measurementUnits
{
  settings::Set(settings::kMeasurementUnits, coreUnits(measurementUnits));
  GetFramework().SetupMeasurementSystem();
}

+ (BOOL)zoomButtonsEnabled
{
  bool enabled = true;
  UNUSED_VALUE(settings::Get(kZoomButtonsEnabledKey, enabled));
  return enabled;
}

+ (void)setZoomButtonsEnabled:(BOOL)zoomButtonsEnabled
{
  settings::Set(kZoomButtonsEnabledKey, static_cast<bool>(zoomButtonsEnabled));
  [MWMMapViewControlsManager manager].zoomHidden = !zoomButtonsEnabled;
}

+ (MWMPlacement)bookmarksTextPlacement
{
  switch (GetFramework().GetBookmarksTextPlacement())
  {
    using settings::Placement;
  case Placement::None: return MWMPlacementNone;
  case Placement::Right: return MWMPlacementRight;
  case Placement::Bottom: return MWMPlacementBottom;
  default: UNREACHABLE();
  }
}

+ (void)setBookmarksTextPlacement:(MWMPlacement)placement
{
  using settings::Placement;
  Placement setting;
  switch (placement)
  {
  case MWMPlacementNone: setting = Placement::None; break;
  case MWMPlacementRight: setting = Placement::Right; break;
  case MWMPlacementBottom: setting = Placement::Bottom; break;
  default: UNREACHABLE();
  }
  GetFramework().SetBookmarksTextPlacement(setting);
}

+ (BOOL)compassCalibrationEnabled
{
  bool enabled = true;
  UNUSED_VALUE(settings::Get(kCompassCalibrationEnabledKey, enabled));
  return enabled;
}

+ (void)setCompassCalibrationEnabled:(BOOL)compassCalibrationEnabled
{
  settings::Set(kCompassCalibrationEnabledKey, static_cast<bool>(compassCalibrationEnabled));
}

+ (MWMTheme)theme
{
  auto ud = NSUserDefaults.standardUserDefaults;
  if (![ud boolForKey:kUDAutoNightModeOff])
    return MWMThemeAuto;
  return static_cast<MWMTheme>([ud integerForKey:kThemeMode]);
}

+ (void)setTheme:(MWMTheme)theme
{
  if ([self theme] == theme)
    return;
  auto ud = NSUserDefaults.standardUserDefaults;
  [ud setInteger:theme forKey:kThemeMode];
  BOOL const autoOff = theme != MWMThemeAuto;
  [ud setBool:autoOff forKey:kUDAutoNightModeOff];
  [MWMThemeManager invalidate];
}

+ (BOOL)routingDisclaimerApproved
{
  bool enabled = false;
  UNUSED_VALUE(settings::Get(kRoutingDisclaimerApprovedKey, enabled));
  return enabled;
}

+ (void)setRoutingDisclaimerApproved
{
  settings::Set(kRoutingDisclaimerApprovedKey, true);
}
+ (NSString *)spotlightLocaleLanguageId
{
  return [NSUserDefaults.standardUserDefaults stringForKey:kSpotlightLocaleLanguageId];
}

+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:spotlightLocaleLanguageId forKey:kSpotlightLocaleLanguageId];
}

+ (BOOL)largeFontSize
{
  return GetFramework().LoadLargeFontsSize();
}
+ (void)setLargeFontSize:(BOOL)largeFontSize
{
  GetFramework().SetLargeFontsSize(static_cast<bool>(largeFontSize));
}

+ (BOOL)transliteration
{
  return GetFramework().LoadTransliteration();
}
+ (void)setTransliteration:(BOOL)transliteration
{
  bool const isTransliteration = static_cast<bool>(transliteration);
  auto & f = GetFramework();
  f.SaveTransliteration(isTransliteration);
  f.AllowTransliteration(isTransliteration);
}

+ (BOOL)map3dBuildingsEnabled
{
  bool allow3d = true, allow3dBuildings = true;
  GetFramework().Load3dMode(allow3d, allow3dBuildings);
  return allow3dBuildings;
}

+ (void)setMap3dBuildingsEnabled:(BOOL)enabled
{
  auto & f = GetFramework();
  bool allow3d = true, allow3dBuildings = true;
  f.Load3dMode(allow3d, allow3dBuildings);
  allow3dBuildings = static_cast<bool>(enabled);
  f.Save3dMode(allow3d, allow3dBuildings);
  f.Allow3dMode(allow3d, allow3dBuildings);
}

+ (BOOL)perspectiveViewEnabled
{
  bool allow3d = true, allow3dBuildings = true;
  GetFramework().Load3dMode(allow3d, allow3dBuildings);
  return allow3d;
}

+ (void)setPerspectiveViewEnabled:(BOOL)enabled
{
  auto & f = GetFramework();
  bool allow3d = true, allow3dBuildings = true;
  f.Load3dMode(allow3d, allow3dBuildings);
  allow3d = static_cast<bool>(enabled);
  f.Save3dMode(allow3d, allow3dBuildings);
  f.Allow3dMode(allow3d, allow3dBuildings);
}

+ (BOOL)autoZoomEnabled
{
  return GetFramework().LoadAutoZoom();
}

+ (void)setAutoZoomEnabled:(BOOL)enabled
{
  auto & f = GetFramework();
  f.AllowAutoZoom(enabled);
  f.SaveAutoZoom(enabled);
}

+ (MWMSettingsPowerManagement)powerManagement
{
  using power_management::Scheme;
  switch (GetFramework().GetPowerManager().GetScheme())
  {
  case Scheme::None: return MWMSettingsPowerManagementNone;
  case Scheme::Normal: return MWMSettingsPowerManagementNormal;
  case Scheme::EconomyMedium: return MWMSettingsPowerManagementEconomyMedium;
  case Scheme::EconomyMaximum: return MWMSettingsPowerManagementEconomyMaximum;
  case Scheme::Auto: return MWMSettingsPowerManagementAuto;
  }
}

+ (void)setPowerManagement:(MWMSettingsPowerManagement)powerManagement
{
  using power_management::Scheme;
  Scheme scheme = Scheme::Auto;
  switch (powerManagement)
  {
  case MWMSettingsPowerManagementNone: scheme = Scheme::None; break;
  case MWMSettingsPowerManagementNormal: scheme = Scheme::Normal; break;
  case MWMSettingsPowerManagementEconomyMedium: scheme = Scheme::EconomyMedium; break;
  case MWMSettingsPowerManagementEconomyMaximum: scheme = Scheme::EconomyMaximum; break;
  case MWMSettingsPowerManagementAuto: scheme = Scheme::Auto; break;
  }
  GetFramework().GetPowerManager().SetScheme(scheme);
}

+ (BOOL)isPowerManagementMaximum
{
  return [self powerManagement] == MWMSettingsPowerManagementEconomyMaximum;
}

+ (BOOL)isTrackWarningAlertShown
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kUDTrackWarningAlertWasShown];
}

+ (void)setTrackWarningAlertShown:(BOOL)shown
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:shown forKey:kUDTrackWarningAlertWasShown];
}

+ (NSString *)donateUrl
{
  auto const url = GetFramework().GetDonateUrl();
  return url.empty() ? nil : @(url.c_str());
}

+ (BOOL)isNY
{
  bool isNY;
  return settings::Get(settings::kNY, isNY) ? isNY : false;
}

+ (BOOL)isShowDownloadedRegions
{
  return GetFramework().IsShowDownloadedRegions();
}

+ (void)setShowDownloadedRegions:(BOOL)isEnabled
{
  GetFramework().SetShowDownloadedRegions(isEnabled);
}

+ (MWMNetworkPolicyPermission)mobileInternetPermission
{
  return MWMNetworkPolicy.sharedPolicy.permission;
}

+ (void)setMobileInternetPermission:(MWMNetworkPolicyPermission)permission
{
  MWMNetworkPolicy.sharedPolicy.permission = permission;
}

+ (BOOL)iCLoudSynchronizationEnabled
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kiCLoudSynchronizationEnabledKey];
}

+ (void)setICLoudSynchronizationEnabled:(BOOL)iCLoudSyncEnabled
{
  [NSUserDefaults.standardUserDefaults setBool:iCLoudSyncEnabled forKey:kiCLoudSynchronizationEnabledKey];
  [NSNotificationCenter.defaultCenter postNotificationName:NSNotification.iCloudSynchronizationDidChangeEnabledState
                                                    object:nil];
}

+ (void)initializeLogging
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ [self setFileLoggingEnabled:[self isFileLoggingEnabled]]; });
}

+ (BOOL)isFileLoggingEnabled
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kUDFileLoggingEnabledKey];
}

+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled
{
  [NSUserDefaults.standardUserDefaults setBool:fileLoggingEnabled forKey:kUDFileLoggingEnabledKey];
  [Logger setFileLoggingEnabled:fileLoggingEnabled];
}

+ (uint64_t)logFileSize
{
  return [Logger getLogFileSize];
}

+ (BOOL)didShowICloudSynchronizationEnablingAlert
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kUDDidShowICloudSynchronizationEnablingAlert];
}

+ (void)setICloudSynchronizationEnablingAlertShown
{
  [NSUserDefaults.standardUserDefaults setBool:YES forKey:kUDDidShowICloudSynchronizationEnablingAlert];
}

+ (BOOL)backgroundTilesEnabled
{
  return GetFramework().IsBackgroundTilesEnabled();
}

+ (void)setBackgroundTilesEnabled:(BOOL)enabled
{
  GetFramework().SetBackgroundTilesEnabled(enabled);
}

+ (NSInteger)backgroundTilesAreaOpacityPct
{
  return static_cast<NSInteger>(GetFramework().GetBackgroundTilesAreaOpacity());
}

+ (NSString *)backgroundTilesURL
{
  return @(Framework::GetBackgroundTilesURL().c_str());
}

+ (NSInteger)backgroundTilesCacheSizeMB
{
  return static_cast<NSInteger>(Framework::GetBackgroundTilesCacheSize());
}

+ (void)setBackgroundTilesEnabled:(BOOL)enabled
                              url:(NSString *)url
                      cacheSizeMB:(NSInteger)cacheSizeMB
                   areaOpacityPct:(NSInteger)areaOpacityPct
{
  GetFramework().SetBackgroundTiles(enabled, url.UTF8String, static_cast<uint32_t>(cacheSizeMB),
                                    static_cast<uint32_t>(areaOpacityPct));
}

+ (BOOL)isWellFormedBackgroundTilesURL:(NSString *)url
{
  return Framework::IsWellFormedBackgroundTilesURL(url.UTF8String);
}

+ (BOOL)canShowCrowdfundingPromo
{
  return GetFramework().CanShowCrowdfundingPromo();
}

+ (void)didShowDonationPage
{
  GetFramework().DidShowDonationPage();
}

+ (void)resetDonations
{
  GetFramework().ResetDonations();
}

@end

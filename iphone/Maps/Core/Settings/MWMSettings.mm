#import "MWMSettings.h"
#import "MWMCoreUnits.h"
#import "MWMMapViewControlsManager.h"
#import "SwiftBridge.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "Flurry.h"

#include "Framework.h"

#include "platform/settings.hpp"

namespace
{
char const * kAdServerForbiddenKey = "AdServerForbidden";
char const * kAutoDownloadEnabledKey = "AutoDownloadEnabled";
char const * kZoomButtonsEnabledKey = "ZoomButtonsEnabled";
char const * kCompassCalibrationEnabledKey = "CompassCalibrationEnabled";
char const * kRoutingDisclaimerApprovedKey = "IsDisclaimerApproved";
char const * kStatisticsEnabledSettingsKey = "StatisticsEnabled";

// TODO(igrechuhin): Remove outdated kUDAutoNightModeOff
NSString * const kUDAutoNightModeOff = @"AutoNightModeOff";
NSString * const kThemeMode = @"ThemeMode";
NSString * const kSpotlightLocaleLanguageId = @"SpotlightLocaleLanguageId";
NSString * const kUDTrackWarningAlertWasShown = @"TrackWarningAlertWasShown";
NSString * const kCrashReportingDisabled = @"CrashReportingDisabled";
}  // namespace

@implementation MWMSettings

+ (BOOL)adServerForbidden
{
  bool adServerForbidden = false;
  UNUSED_VALUE(settings::Get(kAdServerForbiddenKey, adServerForbidden));
  return adServerForbidden;
}

+ (void)setAdServerForbidden:(BOOL)adServerForbidden
{
  settings::Set(kAdServerForbiddenKey, static_cast<bool>(adServerForbidden));
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

+ (BOOL)statisticsEnabled
{
  bool enabled = true;
  UNUSED_VALUE(settings::Get(kStatisticsEnabledSettingsKey, enabled));
  return enabled;
}

+ (void)setStatisticsEnabled:(BOOL)statisticsEnabled
{
  if (statisticsEnabled)
  {
    [Alohalytics enable];
    [Flurry trackPreciseLocation:YES];
  }
  else
  {
    [Alohalytics logEvent:@"statisticsDisabled"];
    [Alohalytics disable];
    [Flurry trackPreciseLocation:NO];
  }
  settings::Set(kStatisticsEnabledSettingsKey, static_cast<bool>(statisticsEnabled));
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

+ (void)setRoutingDisclaimerApproved { settings::Set(kRoutingDisclaimerApprovedKey, true); }
+ (NSString *)spotlightLocaleLanguageId
{
  return [NSUserDefaults.standardUserDefaults stringForKey:kSpotlightLocaleLanguageId];
}

+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:spotlightLocaleLanguageId forKey:kSpotlightLocaleLanguageId];
  [ud synchronize];
}

+ (BOOL)largeFontSize { return GetFramework().LoadLargeFontsSize(); }
+ (void)setLargeFontSize:(BOOL)largeFontSize
{
  bool const isLargeSize = static_cast<bool>(largeFontSize);
  auto & f = GetFramework();
  f.SaveLargeFontsSize(isLargeSize);
  f.SetLargeFontsSize(isLargeSize);
}

+ (BOOL)transliteration { return GetFramework().LoadTransliteration(); }
+ (void)setTransliteration:(BOOL)transliteration
{
  bool const isTransliteration = static_cast<bool>(transliteration);
  auto & f = GetFramework();
  f.SaveTransliteration(isTransliteration);
  f.AllowTransliteration(isTransliteration);
}

+ (BOOL)isTrackWarningAlertShown
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kUDTrackWarningAlertWasShown];
}

+ (void)setTrackWarningAlertShown:(BOOL)shown
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:shown forKey:kUDTrackWarningAlertWasShown];
  [ud synchronize];
}

+ (BOOL)crashReportingDisabled
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:kCrashReportingDisabled];
}

+ (void)setCrashReportingDisabled:(BOOL)disabled
{
  [[NSUserDefaults standardUserDefaults] setBool:disabled forKey:kCrashReportingDisabled];
}
@end

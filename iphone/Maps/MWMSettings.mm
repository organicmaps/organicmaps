#import "MWMSettings.h"
#import "MWMMapViewControlsManager.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/settings.hpp"

namespace
{
char const * kAdForbiddenSettingsKey = "AdForbidden";
char const * kAdServerForbiddenKey = "AdServerForbidden";
char const * kAutoDownloadEnabledKey = "AutoDownloadEnabled";
char const * kZoomButtonsEnabledKey = "ZoomButtonsEnabled";
char const * kCompassCalibrationEnabledKey = "CompassCalibrationEnabled";
char const * kRoutingDisclaimerApprovedKey = "IsDisclaimerApproved";
char const * kStatisticsEnabledSettingsKey = "StatisticsEnabled";

NSString * const kUDAutoNightModeOff = @"AutoNightModeOff";
NSString * const kSpotlightLocaleLanguageId = @"SpotlightLocaleLanguageId";
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

+ (BOOL)adForbidden
{
  bool adForbidden = false;
  UNUSED_VALUE(settings::Get(kAdForbiddenSettingsKey, adForbidden));
  return adForbidden;
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

+ (measurement_utils::Units)measurementUnits
{
  auto units = measurement_utils::Units::Metric;
  UNUSED_VALUE(settings::Get(settings::kMeasurementUnits, units));
  return units;
}

+ (void)setMeasurementUnits:(measurement_utils::Units)measurementUnits
{
  settings::Set(settings::kMeasurementUnits, measurementUnits);
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
  }
  else
  {
    [Alohalytics logEvent:@"statisticsDisabled"];
    [Alohalytics disable];
  }
  settings::Set(kStatisticsEnabledSettingsKey, static_cast<bool>(statisticsEnabled));
}

+ (BOOL)autoNightModeEnabled
{
  return ![[NSUserDefaults standardUserDefaults] boolForKey:kUDAutoNightModeOff];
}

+ (void)setAutoNightModeEnabled:(BOOL)autoNightModeEnabled
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:!autoNightModeEnabled forKey:kUDAutoNightModeOff];
  [ud synchronize];
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
  return [[NSUserDefaults standardUserDefaults] stringForKey:kSpotlightLocaleLanguageId];
}

+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:spotlightLocaleLanguageId forKey:kSpotlightLocaleLanguageId];
  [ud synchronize];
}

@end

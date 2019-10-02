#import "MWMFrameworkHelper.h"

#include <CoreApi/Framework.h>

#include "base/sunrise_sunset.hpp"

#include "map/crown.hpp"

#include "platform/network_policy_ios.h"
#include "platform/local_country_file_utils.hpp"

@implementation MWMFrameworkHelper

+ (void)processFirstLaunch:(BOOL)hasLocation
{
  auto & f = GetFramework();
  if (!hasLocation)
    f.SwitchMyPositionNextMode();
  else
    f.RunFirstLaunchAnimation();
}

+ (void)setVisibleViewport:(CGRect)rect scaleFactor:(CGFloat)scale
{
  CGFloat const x0 = rect.origin.x * scale;
  CGFloat const y0 = rect.origin.y * scale;
  CGFloat const x1 = x0 + rect.size.width * scale;
  CGFloat const y1 = y0 + rect.size.height * scale;
  GetFramework().SetVisibleViewport(m2::RectD(x0, y0, x1, y1));
}

+ (void)setTheme:(MWMTheme)theme
{
  auto & f = GetFramework();

  auto const style = f.GetMapStyle();
  auto const newStyle = ^MapStyle(MWMTheme theme) {
    switch (theme)
    {
    case MWMThemeDay: return MapStyleClear;
    case MWMThemeVehicleDay: return MapStyleVehicleClear;
    case MWMThemeNight: return MapStyleDark;
    case MWMThemeVehicleNight: return MapStyleVehicleDark;
    case MWMThemeAuto: NSAssert(NO, @"Invalid theme"); return MapStyleClear;
    }
  }(theme);

  if (style != newStyle)
    f.SetMapStyle(newStyle);
}

+ (MWMDayTime)daytimeAtLocation:(CLLocation *)location
{
  if (!location)
    return MWMDayTimeDay;
  DayTimeType dayTime = GetDayTime(NSDate.date.timeIntervalSince1970,
                                   location.coordinate.latitude,
                                   location.coordinate.longitude);
  switch (dayTime)
  {
  case DayTimeType::Day:
  case DayTimeType::PolarDay: return MWMDayTimeDay;
  case DayTimeType::Night:
  case DayTimeType::PolarNight: return MWMDayTimeNight;
  }
}

+ (void)createFramework { UNUSED_VALUE(GetFramework()); }

+ (BOOL)canUseNetwork
{
  return network_policy::CanUseNetwork();
}

+ (BOOL)isNetworkConnected
{
  return GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE;
}

+ (BOOL)isWiFiConnected
{
  return GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI;
}

+ (MWMConnectionType)connectionType {
  switch (GetPlatform().ConnectionStatus()) {
    case Platform::EConnectionType::CONNECTION_NONE:
      return MWMConnectionTypeNone;
    case Platform::EConnectionType::CONNECTION_WIFI:
      return MWMConnectionTypeWifi;
    case Platform::EConnectionType::CONNECTION_WWAN:
      return MWMConnectionTypeCellular;
  }
}

+ (MWMMarkGroupID)invalidCategoryId { return kml::kInvalidMarkGroupId; }

+ (NSArray<NSString *> *)obtainLastSearchQueries
{
  NSMutableArray * result = [NSMutableArray array];
  auto const & queries = GetFramework().GetLastSearchQueries();
  for (auto const & item : queries)
  {
    [result addObject:@(item.second.c_str())];
  }
  return [result copy];
}

#pragma mark - Map Interaction

+ (void)zoomMap:(MWMZoomMode)mode
{
  switch(mode) {
    case MWMZoomModeIn:
      GetFramework().Scale(Framework::SCALE_MAG, true);
      break;
    case MWMZoomModeOut:
      GetFramework().Scale(Framework::SCALE_MIN, true);
      break;
  }
}

+ (void)moveMap:(UIOffset)offset
{
  GetFramework().Move(offset.horizontal, offset.vertical, true);
}

+ (void)deactivateMapSelection:(BOOL)notifyUI
{
  GetFramework().DeactivateMapSelection(notifyUI);
}

+ (void)switchMyPositionMode
{
  GetFramework().SwitchMyPositionNextMode();
}

+ (void)stopLocationFollow
{
  GetFramework().StopLocationFollow();
}

+ (void)rotateMap:(double)azimuth animated:(BOOL)isAnimated {
  GetFramework().Rotate(azimuth, isAnimated);
}

+ (void)updatePositionArrowOffset:(BOOL)useDefault offset:(int)offsetY {
  GetFramework().UpdateMyPositionRoutingOffset(useDefault, offsetY);
}

+ (BOOL)shouldShowCrown {
  return crown::NeedToShow(GetFramework().GetPurchase());
}

@end

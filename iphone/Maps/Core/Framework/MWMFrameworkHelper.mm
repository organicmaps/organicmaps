#import "MWMFrameworkHelper.h"
#import "MWMLocationManager.h"
#import "MapViewController.h"

#include "Framework.h"

#include "base/sunrise_sunset.hpp"

@implementation MWMFrameworkHelper

+ (void)processFirstLaunch
{
  auto & f = GetFramework();
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    f.SwitchMyPositionNextMode();
  else
    f.RunFirstLaunchAnimation();
}

+ (void)setVisibleViewport:(CGRect)rect
{
  CGFloat const scale = [MapViewController controller].view.contentScaleFactor;
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

+ (MWMDayTime)daytime
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return MWMDayTimeDay;
  auto const coord = lastLocation.coordinate;
  auto const timeUtc = static_cast<time_t>(NSDate.date.timeIntervalSince1970);
  auto const dayTime = GetDayTime(timeUtc, coord.latitude, coord.longitude);
  switch (dayTime)
  {
  case DayTimeType::Day:
  case DayTimeType::PolarDay: return MWMDayTimeDay;
  case DayTimeType::Night:
  case DayTimeType::PolarNight: return MWMDayTimeNight;
  }
}

+ (void)createFramework { UNUSED_VALUE(GetFramework()); }
@end

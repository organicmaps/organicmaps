#import "Preferences.h"

#include "Framework.h"

#include "platform/settings.hpp"

@implementation Preferences

// TODO: Export this logic to C++

+ (void)setup
{
  Settings::Units u;
  if (!Settings::Get(Settings::kMeasurementUnits, u))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    u = isMetric ? Settings::Metric : Settings::Foot;
    Settings::Set(Settings::kMeasurementUnits, u);
  }
  GetFramework().SetupMeasurementSystem();
}

@end
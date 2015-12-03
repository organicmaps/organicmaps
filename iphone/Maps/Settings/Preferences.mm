#import "Preferences.h"

#include "Framework.h"

#include "platform/settings.hpp"

@implementation Preferences

// TODO: Export this logic to C++

+ (void)setup
{
  Settings::Units u;
  string const units = "Units";
  if (!Settings::Get(units, u))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    u = isMetric ? Settings::Metric : Settings::Foot;
    Settings::Set(units, u);
  }
  GetFramework().SetupMeasurementSystem();
}

@end
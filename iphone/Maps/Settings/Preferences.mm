#import "Preferences.h"

#include "Framework.h"

#include "platform/settings.hpp"

@implementation Preferences

// TODO: Export this logic to C++

+ (void)setup
{
  settings::Units u;
  if (!settings::Get(settings::kMeasurementUnits, u))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    u = isMetric ? settings::Metric : settings::Foot;
    settings::Set(settings::kMeasurementUnits, u);
  }
  GetFramework().SetupMeasurementSystem();
}

@end
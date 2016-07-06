#import "Preferences.h"

#include "Framework.h"

#include "platform/settings.hpp"

@implementation Preferences

// TODO: Export this logic to C++

+ (void)setup
{
  auto units = measurement_utils::Units::Metric;
  if (!settings::Get(settings::kMeasurementUnits, units))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    units = isMetric ? measurement_utils::Units::Metric : measurement_utils::Units::Imperial;
    settings::Set(settings::kMeasurementUnits, units);
  }
  GetFramework().SetupMeasurementSystem();
}

@end
#import "MWMGeoUtil.h"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "platform/locale.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

@implementation Measure

// Alternative native implementation.
// It has the issue: some localized unit are too long even in .short style. E.g. speed for RU.
/*
  let imperial = Settings.measurementUnits() == .imperial
  var speedMeasurement = Measurement(value: speed, unit: UnitSpeed.metersPerSecond)
  speedMeasurement.convert(to: imperial ? UnitSpeed.milesPerHour : UnitSpeed.kilometersPerHour)

  let formatter = MeasurementFormatter()
  formatter.unitOptions = .providedUnit
  formatter.numberFormatter.maximumFractionDigits = 0
  formatter.unitStyle = .short

  if speedMeasurement.value < 10
  {
    formatter.numberFormatter.minimumFractionDigits = 1
    formatter.numberFormatter.maximumFractionDigits = 1
  }

  let speedString = formatter.string(from: speedMeasurement)
*/

- (NSString *)valueAsString
{
  auto const outString = measurement_utils::ToStringPrecision(self.value, self.value >= 10.0 ? 0 : 1);
  return [NSString stringWithUTF8String:outString.c_str()];
}

- (instancetype)initAsSpeed:(double)mps
{
  self = [super init];
  if (self)
  {
    auto units = measurement_utils::Units::Metric;
    settings::TryGet(settings::kMeasurementUnits, units);
    _value = measurement_utils::MpsToUnits(mps, units);

    _unit = @(platform::GetLocalizedSpeedUnits(units).c_str());
  }
  return self;
}

@end

@implementation MWMGeoUtil

+ (float)angleAtPoint:(CLLocationCoordinate2D)p1 toPoint:(CLLocationCoordinate2D)p2
{
  auto mp1 = mercator::FromLatLon(p1.latitude, p1.longitude);
  auto mp2 = mercator::FromLatLon(p2.latitude, p2.longitude);
  return ang::AngleTo(mp1, mp2);
}

+ (NSString *)formattedOsmLinkForCoordinate:(CLLocationCoordinate2D)coordinate zoomLevel:(int)zoomLevel
{
  auto const link = measurement_utils::FormatOsmLink(coordinate.latitude, coordinate.longitude, zoomLevel);
  return [NSString stringWithCString:link.c_str() encoding:NSUTF8StringEncoding];
}

@end

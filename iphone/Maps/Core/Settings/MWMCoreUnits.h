#include "platform/measurement_utils.hpp"

static inline measurement_utils::Units coreUnits(MWMUnits units)
{
  switch (units)
  {
  case MWMUnitsMetric: return measurement_utils::Units::Metric;
  case MWMUnitsImperial: return measurement_utils::Units::Imperial;
  }
}

static inline MWMUnits mwmUnits(measurement_utils::Units units)
{
  switch (units)
  {
  case measurement_utils::Units::Metric: return MWMUnitsMetric;
  case measurement_utils::Units::Imperial: return MWMUnitsImperial;
  }
}

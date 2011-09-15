#include "measurement_utils.hpp"

#include "../base/string_utils.hpp"
#include "../platform/settings.hpp"

namespace MeasurementUtils
{

string FormatDistanceImpl(double m, bool & drawDir,
                          char const * high, char const * low, double highF, double lowF)
{
  double const lowV = m / lowF;
  drawDir = true;
  if (lowV < 1.0)
  {
    drawDir = false;
    return string("0") + low;
  }

  if (m >= highF)
    return strings::to_string(m / highF) + high;
  else
    return strings::to_string(lowV) + low;
}

string FormatDistance(double m, bool & drawDir)
{
  using namespace Settings;
  Units u = Metric;
  Settings::Get("Units", u);

  switch (u)
  {
  case Yard: return FormatDistanceImpl(m, drawDir, " mi", " yd", 1609.344, 0.9144);
  case Foot: return FormatDistanceImpl(m, drawDir, " mi", " ft", 1609.344, 0.3048);
  default: return FormatDistanceImpl(m, drawDir, " km", " m", 1000.0, 1.0);
  }
}

} // namespace MeasurementUtils

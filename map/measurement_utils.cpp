#include "measurement_utils.hpp"

#include "../base/string_utils.hpp"
#include "../platform/settings.hpp"

namespace MeasurementUtils
{

bool FormatDistanceImpl(double m, string & res,
                          char const * high, char const * low, double highF, double lowF)
{
  double const lowV = m / lowF;
  if (lowV < 1.0)
  {
    res = string("0") + low;
    return false;
  }

  if (m >= highF)
    res = strings::to_string(m / highF) + high;
  else
    res = strings::to_string(lowV) + low;

  return true;
}

bool FormatDistance(double m, string & res)
{
  using namespace Settings;
  Units u = Metric;
  Settings::Get("Units", u);

  switch (u)
  {
  case Yard: return FormatDistanceImpl(m, res, " mi", " yd", 1609.344, 0.9144);
  case Foot: return FormatDistanceImpl(m, res, " mi", " ft", 1609.344, 0.3048);
  default: return FormatDistanceImpl(m, res, " km", " m", 1000.0, 1.0);
  }
}

} // namespace MeasurementUtils

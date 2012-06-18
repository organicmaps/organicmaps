#include "measurement_utils.hpp"

#include "../platform/settings.hpp"

#include "../base/string_utils.hpp"


namespace MeasurementUtils
{

string ToStringPrecision(double d, int pr)
{
  stringstream ss;
  ss.precision(pr);
  ss << std::fixed << d;
  return ss.str();
}

bool FormatDistanceImpl(double m, string & res,
                        char const * high, char const * low,
                        double highF, double lowF)
{
  double const lowV = m / lowF;
  if (lowV < 1.0)
  {
    res = string("0") + low;
    return false;
  }

  if (m >= highF)
  {
    double const v = m / highF;
    res = ToStringPrecision(v, v >= 10.0 ? 0 : 1) + high;
  }
  else
    res = ToStringPrecision(lowV, 0) + low;

  return true;
}

bool FormatDistance(double m, string & res)
{
  using namespace Settings;
  Units u = Metric;
  Settings::Get("Units", u);

  /// @todo Put string units resources.
  switch (u)
  {
  case Yard: return FormatDistanceImpl(m, res, " mi", " yd", 1609.344, 0.9144);
  case Foot: return FormatDistanceImpl(m, res, " mi", " ft", 1609.344, 0.3048);
  default: return FormatDistanceImpl(m, res, " km", " m", 1000.0, 1.0);
  }
}

} // namespace MeasurementUtils

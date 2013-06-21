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

string FormatLatLonAsDMSImpl(string const & posPost, string const & negPost,
                             double value,bool roundSec)
{
   double i = 0.0;
   double d = 0.0;
   string postfix;
   ostringstream sstream;
   sstream << setfill('0');

   // Degreess
   d = modf(fabs(value), &i);
   sstream << setw(2) << i << "°";
   // Minutes
   d = modf(d * 60, &i);
   sstream << setw(2) << i << "′";
   // Seconds
   if (roundSec)
   {
     d = modf(round(d * 60), &i);
     sstream << setw(2) << i;
   }
   else
   {
     d = modf(d * 60, &i);
     sstream << setw(2) << setprecision(2) << i;
     if (d > 1e-5)
     {
       ostringstream tstream;
       tstream << setprecision(4) << d;
       string dStr = tstream.str().substr(1, 5);
       sstream << dStr;
     }
   }
   sstream << "″";

   if (value > 0)
     postfix = posPost;
   else if (value < 0)
     postfix = negPost;
   sstream << postfix;

   return sstream.str();
}

string FormatLatLonAsDMS(double lat, double lon, bool roundSecToInt)
{
  return FormatLatLonAsDMSImpl("N", "S", lat, roundSecToInt) + " "  + FormatLatLonAsDMSImpl("E", "W", lon, roundSecToInt);
}

} // namespace MeasurementUtils

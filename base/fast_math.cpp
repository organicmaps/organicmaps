// Separate cpp for different compiler flags.
#include <cmath>
#include <limits>

namespace math
{
double Nan()
{
  return std::numeric_limits<double>::quiet_NaN();
}
double Infinity()
{
  return std::numeric_limits<double>::infinity();
}
bool is_finite(double t)
{
  return std::isfinite(t);
}
} // namespace base

#include "geometry/oblate_spheroid.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

using namespace std;

namespace
{
// Length of semi-major axis of the spheroid WGS-84.
double constexpr kA = 6378137.0;

// Flattening of the spheroid.
double constexpr kF = 1.0 / 298.257223563;

// Length of semi-minor axis of the spheroid ~ 6356752.31424.
double constexpr kB = (1.0 - kF) * kA;

// Desired degree of accuracy for convergence of Vincenty's formulae.
double constexpr kEps = 1e-10;

// Maximum iterations of distance evaluation.
int32_t constexpr kIterations = 10;

/// \brief Calculates latitude on the auxiliary sphere for |angleRad| latitude on a spheroid.
double ReducedLatitude(double angleRad) { return atan((1.0 - kF) * tan(angleRad)); }
}

namespace oblate_spheroid
{
double GetDistance(ms::LatLon const & point1, ms::LatLon const & point2)
{
  m2::PointD const p1 = {base::DegToRad(point1.m_lon), base::DegToRad(point1.m_lat)};
  m2::PointD const p2 = {base::DegToRad(point2.m_lon), base::DegToRad(point2.m_lat)};
  double const U1 = ReducedLatitude(p1.y);
  double const U2 = ReducedLatitude(p2.y);
  double const sinU1 = sin(U1);
  double const cosU1 = cos(U1);
  double const sinU2 = sin(U2);
  double const cosU2 = cos(U2);

  // Difference in longitude of two points.
  double const L = p2.x - p1.x;
  // Difference in longitude on the auxiliary sphere.
  double lambda = L;
  double lambdaPrev = std::numeric_limits<double>::max();
  uint32_t iterations = kIterations;
  double sinSigma = NAN;
  double cosSigma = NAN;
  double sigma = NAN;
  double cosAlphaSquare = NAN;
  double cosDoubleSigmaMid = NAN;
  double cosDoubleSigmaMidSquare = NAN;

  while (iterations > 0 && !base::AlmostEqualAbs(lambda, lambdaPrev, kEps))
  {
    --iterations;
    sinSigma = sqrt(pow(cosU2 * sin(lambda), 2) +
                    pow((cosU1 * sinU2 - sinU1 * cosU2 * cos(lambda)), 2));
    cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cos(lambda);
    sigma = atan2(sinSigma, cosSigma);
    double const sinAlpha = cosU1 * cosU2 * sin(lambda) / sinSigma;
    cosAlphaSquare = (1 - pow(sinAlpha, 2));

    // Cosine of SigmaMid - angular separation between the midpoint of the line and the equator.
    cosDoubleSigmaMid = cos(sigma) - 2 * sinU1 * sinU2 / cosAlphaSquare;
    if (isnan(cosDoubleSigmaMid))
      cosDoubleSigmaMid = 0.0;
    cosDoubleSigmaMidSquare = pow(cosDoubleSigmaMid, 2);

    double const C = (kF / 16.0) * cosAlphaSquare * (4.0 + kF * (4.0 - 3.0 * cosAlphaSquare));

    lambdaPrev = lambda;
    lambda = L + (1 - C) * kF * sinAlpha *
                     (sigma +
                      C * sinSigma *
                          (cosDoubleSigmaMid + C * cosSigma * (-1 + 2 * cosDoubleSigmaMidSquare)));
  }

  // Fallback solution.
  if (!base::AlmostEqualAbs(lambda, lambdaPrev, kEps))
    return ms::DistanceOnEarth(point1, point2);

  static double constexpr aSquare = kA * kA;
  static double constexpr bSquare = kB * kB;

  double uSquare = cosAlphaSquare * (aSquare - bSquare) / bSquare;

  double A = 1.0 + (uSquare / 16384.0) *
                       (4096.0 + uSquare * (-768.0 + uSquare * (320.0 - 175.0 * uSquare)));

  double B = (uSquare / 1024.0) * (256.0 + uSquare * (-128.0 + uSquare * (74.0 - 47 * uSquare)));

  double deltaSigma =
      B * sinSigma *
      (cosDoubleSigmaMid + 0.25 * B *
                               (cosSigma * (-1.0 + 2.0 * cosDoubleSigmaMidSquare) -
                                (B / 6.0) * cosDoubleSigmaMid * (-3.0 + 4.0 * pow(sinSigma, 2)) *
                                    (-3.0 + 4 * cosDoubleSigmaMidSquare)));

  return kB * A * (sigma - deltaSigma);
}
} // namespace oblate_spheroid

#include "compass_filter.hpp"
#include "location_state.hpp"

#include "../geometry/angles.hpp"

#include "../base/logging.hpp"

#include "../platform/location.hpp"

CompassFilter::CompassFilter()
{
  m_headingRad = m_smoothedHeadingRad = 0;
  m_headingHalfErrorRad = 0;

  m_smoothingThreshold = ang::DegreeToRad(10);
  m_lowPassKoeff = 0.5;
}

void CompassFilter::OnCompassUpdate(location::CompassInfo const & info)
{
  // Avoid situations when offset between magnetic north and true north is too small
  double const MIN_SECTOR_RAD = ang::DegreeToRad(10); //< 10 degrees threshold

  double newHeadingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);
  double newHeadingDelta = fabs(newHeadingRad - m_headingRad);
  double newHeadingHalfErrorRad = (info.m_accuracy < MIN_SECTOR_RAD ? MIN_SECTOR_RAD : info.m_accuracy);

#ifdef OMIM_OS_IPHONE

  // On iOS we shouldn't smooth the compass values.

  m_headingRad = newHeadingRad;

#else
  // if new heading lies outside the twice headingError radius we immediately accept it
  if (newHeadingDelta > m_headingHalfErrorRad * 2)
  {
    m_headingRad = newHeadingRad;
    m_smoothedHeadingRad = newHeadingRad;
  }
  else
  {
    // else we smooth the received value with the following formula
    // O(n) = O(n-1) + k * (I - O(n - 1));
    m_smoothedHeadingRad = m_smoothedHeadingRad + m_lowPassKoeff * (newHeadingRad - m_smoothedHeadingRad);

    // if the change is too small we won't change the compass value
    if (newHeadingDelta > m_smoothingThreshold)
      m_headingRad = m_smoothedHeadingRad;
  }

#endif

  m_headingHalfErrorRad = newHeadingHalfErrorRad;
  m_smoothingThreshold = m_headingHalfErrorRad * 2;
}

double CompassFilter::GetHeadingRad() const
{
  return m_headingRad;
}

double CompassFilter::GetHeadingHalfErrorRad() const
{
  return m_headingHalfErrorRad;
}


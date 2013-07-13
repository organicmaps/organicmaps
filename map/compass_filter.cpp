#include "compass_filter.hpp"
#include "location_state.hpp"

#include "../geometry/angles.hpp"

#include "../base/logging.hpp"

#include "../platform/location.hpp"


#define LOW_PASS_FACTOR 0.5

CompassFilter::CompassFilter()
{
  m_headingRad = m_smoothedHeadingRad = 0;

  // Set hard smoothing treshold constant to 10 degrees.
  // We can't assign it from location::CompassInfo::accuracy, because actually it's a
  // declination between magnetic and true north in Android
  // (and it may be very large in particular places on the Earth).
  m_smoothingThreshold = my::DegToRad(10);
}

void CompassFilter::OnCompassUpdate(location::CompassInfo const & info)
{
  double const newHeadingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);

#ifdef OMIM_OS_IPHONE

  // On iOS we shouldn't smooth the compass values.

  m_headingRad = newHeadingRad;

#else

  double const newHeadingDelta = fabs(newHeadingRad - m_headingRad);

  // if new heading lies outside the twice treshold radius we immediately accept it
  if (newHeadingDelta > 2.0 * m_smoothingThreshold)
  {
    m_headingRad = newHeadingRad;
    m_smoothedHeadingRad = newHeadingRad;
  }
  else
  {
    // else we smooth the received value with the following formula
    // O(n) = O(n-1) + k * (I - O(n - 1));
    m_smoothedHeadingRad = m_smoothedHeadingRad + LOW_PASS_FACTOR * (newHeadingRad - m_smoothedHeadingRad);

    // if the change is too small we won't change the compass value
    if (newHeadingDelta > m_smoothingThreshold)
      m_headingRad = m_smoothedHeadingRad;
  }

#endif
}

double CompassFilter::GetHeadingRad() const
{
  return m_headingRad;
}

double CompassFilter::GetHeadingHalfErrorRad() const
{
  return m_smoothingThreshold;
}

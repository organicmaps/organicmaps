#include "compass_filter.hpp"
#include "location_state.hpp"
#include "../base/logging.hpp"

#include "../platform/location.hpp"

CompassFilter::CompassFilter(location::State * state)
{
  m_state = state;

  m_headingRad = m_smoothedHeadingRad = 0;
  m_headingHalfErrorRad = 0;

  m_smoothingThreshold = 10 * math::pi / 180.0;
  m_lowPassKoeff = 0.5;
}

void CompassFilter::OnCompassUpdate(location::CompassInfo const & info)
{
  double newHeadingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);
  double newHeadingDelta = fabs(newHeadingRad - m_headingRad);

/*
  LOG(LINFO, ("Accuracy: ", int(ang::RadToDegree(info.m_accuracy)),
              ", Heading: ", int(ang::RadToDegree(newHeadingRad)),
              ", Delta: ", int(ang::RadToDegree(newHeadingRad - m_headingRad))));
 */

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

  // Avoid situations when offset between magnetic north and true north is too small
  double const MIN_SECTOR_RAD = math::pi / 18.0; //< 10 degrees threshold

  double oldHeadingHalfErrorRad = m_headingHalfErrorRad;

  m_headingHalfErrorRad = (info.m_accuracy < MIN_SECTOR_RAD ? MIN_SECTOR_RAD : info.m_accuracy);
  m_smoothingThreshold = m_headingHalfErrorRad * 2;

  /// re-caching threshold for compass accuracy is 5 degrees.
  double reCachingThreshold = 5 * math::pi / 180.0;

  if (fabs(oldHeadingHalfErrorRad - m_headingHalfErrorRad) > reCachingThreshold)
    m_state->setIsDirtyDrawing(true);

  m_state->CheckFollowCompass();
}

double CompassFilter::GetHeadingRad() const
{
  return m_headingRad;
}

double CompassFilter::GetHeadingHalfErrorRad() const
{
  return m_headingHalfErrorRad;
}


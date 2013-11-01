#include "compass_filter.hpp"

//#include "../geometry/angles.hpp"

#include "../platform/location.hpp"


/*
namespace
{
  double const LOW_PASS_FACTOR = 0.5;
  double const TRASHOLD = my::DegToRad(15.0);
}
*/

CompassFilter::CompassFilter()
  : m_headingRad(0.0)
{
}

void CompassFilter::OnCompassUpdate(location::CompassInfo const & info)
{
  double const heading = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);

//#ifdef OMIM_OS_IPHONE

  // On iOS we shouldn't smooth the compass values.

  m_headingRad = heading;

/*
#else

  // if new heading lies outside the twice treshold radius we immediately accept it
  if (fabs(ang::GetShortestDistance(m_headingRad, heading)) >= TRASHOLD)
  {
    m_headingRad = heading;
  }
  else
  {
    // else we smooth the received value with the following formula
    // O(n) = O(n-1) + k * (I - O(n - 1));
    m_headingRad = m_headingRad + LOW_PASS_FACTOR * (heading - m_headingRad);
  }

#endif
*/
}

double CompassFilter::GetHeadingRad() const
{
  return m_headingRad;
}

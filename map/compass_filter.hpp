#pragma once

namespace location
{
  class CompassInfo;
}

class CompassFilter
{
private:
  double m_headingRad;

  // Compass smoothing parameters
  // We're using technique described in
  // http://windowsteamblog.com/windows_phone/b/wpdev/archive/2010/09/08/using-the-accelerometer-on-windows-phone-7.aspx
  // In short it's a combination of low-pass filter to smooth the
  // small orientation changes and a threshold filter to get big changes fast.
  // @{
  // k in the following formula: O(n) = O(n-1) + k * (I - O(n - 1));
  //double m_lowPassKoeff;
  // smoothed heading angle. doesn't always correspond to the m_headingAngle
  // as we change the heading angle only if the delta between
  // smoothedHeadingRad and new heading value is bigger than smoothingThreshold.
  double m_smoothedHeadingRad;
  double m_smoothingThreshold;
  // @}

public:

  // Constructor
  CompassFilter();
  // Getting new compass value
  void OnCompassUpdate(location::CompassInfo const & info);
  // get heading angle in radians.
  double GetHeadingRad() const;
  // get half of heading error in radians.
  double GetHeadingHalfErrorRad() const;
};

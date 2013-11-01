#pragma once

namespace location { class CompassInfo; }

class CompassFilter
{
  double m_headingRad;

public:
  CompassFilter();

  // Getting new compass value
  void OnCompassUpdate(location::CompassInfo const & info);
  // get heading angle in radians.
  double GetHeadingRad() const;
};

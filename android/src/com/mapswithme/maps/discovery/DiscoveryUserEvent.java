package com.mapswithme.maps.discovery;

import com.mapswithme.maps.metrics.UserActionsLogger;

public enum DiscoveryUserEvent
{
  HOTELS_CLICKED,
  ATTRACTIONS_CLICKED,
  CAFES_CLICKED,
  LOCALS_CLICKED,

  MORE_HOTELS_CLICKED,
  MORE_ATTRACTIONS_CLICKED,
  MORE_CAFES_CLICKED,
  MORE_LOCALS_CLICKED,

  PROMO_CLICKED,
  MORE_PROMO_CLICKED,

  /* Must be last */
  STUB
      {
        @Override
        public void log()
        {
          /* Do nothing */
        }
      };

  public void log()
  {
    UserActionsLogger.logDiscoveryItemClickedEvent(this);
  }
}

package com.mapswithme.maps.subway;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.maps.traffic.TrafficManager;

public enum Mode
{
  TRAFFIC
      {
        @Override
        public boolean isEnabled(@NonNull Context context)
        {
          return TrafficManager.INSTANCE.isEnabled();
        }
      },
  SUBWAY
      {
        @Override
        public boolean isEnabled(@NonNull Context context)
        {
          return SubwayManager.from(context).isEnabled();
        }
      };

  public abstract boolean isEnabled(@NonNull Context context);
}

package com.mapswithme.maps.maplayer.subway;

import android.support.annotation.NonNull;
import android.widget.Toast;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

public enum TransitSchemeState
{
  DISABLED,
  ENABLED
      {
        @Override
        public void onReceived(@NonNull MwmApplication app)
        {
          Statistics.INSTANCE.trackSubwayEvent(Statistics.ParamValue.SUCCESS);
        }
      },
  NO_DATA
      {
        @Override
        public void onReceived(@NonNull MwmApplication app)
        {
          Toast.makeText(app, R.string.subway_data_unavailable, Toast.LENGTH_SHORT).show();
          Statistics.INSTANCE.trackSubwayEvent(Statistics.ParamValue.UNAVAILABLE);
        }
      };

  @NonNull
  public static TransitSchemeState makeInstance(int index)
  {
    if (index < 0 || index >= TransitSchemeState.values().length)
      throw new IllegalArgumentException("No value for index = " + index);
    return TransitSchemeState.values()[index];
  }

  public void onReceived(@NonNull MwmApplication app)
  {
    /* Do nothing by default */
  }
}

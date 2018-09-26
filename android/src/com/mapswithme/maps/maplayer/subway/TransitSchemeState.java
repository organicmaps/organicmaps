package com.mapswithme.maps.maplayer.subway;

import android.content.Context;
import android.support.annotation.NonNull;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

enum TransitSchemeState
{
  DISABLED,
  ENABLED
      {
        @Override
        public void activate(@NonNull Context context)
        {
          Statistics.INSTANCE.trackSubwayEvent(Statistics.ParamValue.SUCCESS);
        }
      },
  NO_DATA
      {
        @Override
        public void activate(@NonNull Context context)
        {
          Toast.makeText(context, R.string.subway_data_unavailable, Toast.LENGTH_SHORT).show();
          Statistics.INSTANCE.trackSubwayEvent(Statistics.ParamValue.UNAVAILABLE);
        }
      };

  void activate(@NonNull Context context)
  {
    /* Do nothing by default */
  }
}

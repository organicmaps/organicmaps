package com.mapswithme.maps.maplayer.isolines;

import android.content.Context;
import android.widget.Toast;

import androidx.annotation.NonNull;
import com.mapswithme.maps.R;

public enum IsolinesState
{
  DISABLED,
  ENABLED,
  EXPIREDDATA
      {
        @Override
        public void activate(@NonNull Context context)
        {
          Toast.makeText(context, R.string.isolines_activation_error_dialog, Toast.LENGTH_SHORT)
               .show();
        }
      },
  NODATA
      {
        @Override
        public void activate(@NonNull Context context)
        {
          Toast.makeText(context, R.string.isolines_location_error_dialog, Toast.LENGTH_SHORT)
               .show();
        }
      };

  public void activate(@NonNull Context context)
  {
    /* Do nothing by default */
  }
}

package com.mapswithme.maps.maplayer.guides;

import android.content.Context;
import android.widget.Toast;

import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public enum GuidesState
{
  DISABLED,
  ENABLED,
  HAS_DATA
      {
        @Override
        public void activate(@NonNull Context context)
        {
          UiUtils.showToastAtTop(context, R.string.routes_layer_is_on_toast);
        }
      },
  NO_DATA
      {
        @Override
        public void activate(@NonNull Context context)
        {
          Toast.makeText(context, R.string.no_routes_in_the_area_toast, Toast.LENGTH_SHORT).show();
        }
      },
  NETWORK_ERROR,
  FATAL_NETWORK_ERROR;

  public void activate(@NonNull Context context)
  {
  }
}

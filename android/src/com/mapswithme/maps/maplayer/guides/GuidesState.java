package com.mapswithme.maps.maplayer.guides;

import android.content.Context;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public enum GuidesState
{
  DISABLED,
  ENABLED,
  HAS_DATA
      {
        @Override
        public void activate(@NonNull Context context, @Nullable View viewAbove, @Nullable View view)
        {
          if (!SharedPropertiesUtils.shouldShowHowToUseGuidesLayerToast(context))
            return;

          if (view != null)
            Utils.showSnackbar(context, view, viewAbove, R.string.routes_layer_is_on_toast);
          else
            UiUtils.showToastAtTop(context, R.string.routes_layer_is_on_toast);
        }
      },
  NO_DATA
      {
        @Override
        public void activate(@NonNull Context context, @Nullable View view, @Nullable View viewAbove)
        {
          if (view != null)
            Utils.showSnackbar(context, view, viewAbove, R.string.no_routes_in_the_area_toast);
          else
            Toast.makeText(context, R.string.no_routes_in_the_area_toast, Toast.LENGTH_SHORT)
                 .show();
        }
      },
  NETWORK_ERROR
      {
        @Override
        public void activate(@NonNull Context context, @Nullable View view, @Nullable View viewAbove)
        {
          if (view != null)
            Utils.showSnackbar(context, view, viewAbove, R.string.connection_error_toast_guides);
          else
            Toast.makeText(context, R.string.connection_error_toast_guides, Toast.LENGTH_SHORT)
                 .show();
        }
      },
  FATAL_NETWORK_ERROR;

  public void activate(@NonNull Context context, @Nullable View viewAbove, @Nullable View view)
  {
    /* Do nothing by default */
  }
}

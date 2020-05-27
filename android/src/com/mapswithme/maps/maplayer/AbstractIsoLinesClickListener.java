package com.mapswithme.maps.maplayer;

import android.view.View;
import android.widget.Toast;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.isolines.IsolinesManager;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;

public abstract class AbstractIsoLinesClickListener extends DefaultClickListener
{

  public AbstractIsoLinesClickListener(@NonNull LayersAdapter adapter)
  {
    super(adapter);
  }

  @CallSuper
  @Override
  public void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item)
  {
    if (IsolinesManager.from(v.getContext()).shouldShowNotification())
    {
      Toast.makeText(v.getContext(), R.string.isolines_toast_zooms_1_10, Toast.LENGTH_SHORT).show();
      Statistics.INSTANCE.trackEvent(Statistics.EventName.MAP_TOAST_SHOW,
                                     Collections.singletonMap(Statistics.EventParam.TYPE,
                                                              Statistics.ParamValue.ISOLINES));
    }
  }
}

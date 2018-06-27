package com.mapswithme.maps.traffic;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.BottomSheetItem;
import com.mapswithme.maps.subway.SubwayManager;

public class TrafficItem implements BottomSheetItem
{
  @Override
  public boolean isSelected(@NonNull Context context) {
    return !SubwayManager.from(context).isEnabled()
           && TrafficManager.INSTANCE.isEnabled();
  }

  @Override
  public void onSelected(@NonNull MwmActivity activity)
  {
    TrafficManager.INSTANCE.toggle();
    SubwayManager.from(activity).setEnabled(false);
    activity.onTrafficModeSelected();
  }

  @Override
  public int getDrawableResId(@NonNull Context context)
  {
    return TrafficManager.INSTANCE.getIconRes();
  }

  @Override
  public int getTitleResId()
  {
    return R.string.button_layer_traffic;
  }
}

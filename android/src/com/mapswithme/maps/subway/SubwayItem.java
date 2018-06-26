package com.mapswithme.maps.subway;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.BottomSheetItem;
import com.mapswithme.maps.traffic.TrafficManager;

class SubwayItem implements BottomSheetItem
{
  @Override
  public void onSelected(@NonNull MwmActivity activity)
  {
    SubwayManager subway = SubwayManager.from(activity);
    subway.toggle();
    if (subway.isEnabled())
      TrafficManager.INSTANCE.setEnabled(false);

    activity.onSubwayModeSelected();
  }

  @Override
  public int getDrawableResId(@NonNull Context context)
  {
    return SubwayManager.from(context).getIconRes();
  }

  @Override
  public int getTitleResId()
  {
    return R.string.about;
  }

  @Override
  public boolean isSelected(@NonNull Context context)
  {
    return SubwayManager.from(context).isEnabled();
  }
}

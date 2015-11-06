package com.mapswithme.maps.routing;

import android.view.View;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class RoutingPlanInplace extends RoutingPlanController
{
  public RoutingPlanInplace(MwmActivity activity)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity);
  }

  public void show(boolean show)
  {
    if (show == (mFrame.getVisibility() == View.VISIBLE))
      return;

    UiUtils.showIf(show, mFrame);
    if (show)
      updatePoints();
  }
}

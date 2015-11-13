package com.mapswithme.maps.routing;

import android.view.View;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  public RoutingPlanInplaceController(MwmActivity activity)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity);
  }

  public void show(boolean show)
  {
    if (show == UiUtils.isVisible(mFrame))
      return;

    UiUtils.showIf(show, mFrame);
    if (show)
      updatePoints();
  }

  public void setStartButton()
  {
    final MwmActivity activity = (MwmActivity)mActivity;

    View start = activity.getMainMenu().getRouteStartButton();
    RoutingController.get().setStartButton(start);
    start.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        activity.closeMenuAndRun(AlohaHelper.ROUTING_GO, new Runnable()
        {
          @Override
          public void run()
          {
            RoutingController.get().start();
          }
        });
      }
    });
  }
}

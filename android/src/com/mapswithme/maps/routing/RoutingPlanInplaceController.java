package com.mapswithme.maps.routing;

import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  private static final String STATE_OPEN = "slots panel open";
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude chart shown";

  private Boolean mSlotsRestoredState;

  private void updateStatusBarColor()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      mActivity.getWindow().setStatusBarColor(ThemeUtils.getColor(mActivity, UiUtils.isVisible(mFrame) ? R.attr.statusBar
                                                                                                       : android.R.attr.colorPrimaryDark));
  }

  public RoutingPlanInplaceController(MwmActivity activity)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity);
    updateStatusBarColor();
  }

  public void show(boolean show)
  {
    if (show == UiUtils.isVisible(mFrame))
      return;

    if (show)
    {
      final MapObject start = RoutingController.get().getStartPoint();
      final MapObject end = RoutingController.get().getEndPoint();
      boolean open = (mSlotsRestoredState == null
                        ? (!MapObject.isOfType(MapObject.MY_POSITION, start) || end == null)
                        : mSlotsRestoredState);
      showSlots(open, false);
      mSlotsRestoredState = null;
    }

    UiUtils.showIf(show, mFrame);
    updateStatusBarColor();
    if (show)
      updatePoints();
  }

  public void setStartButton()
  {
    final MwmActivity activity = (MwmActivity) mActivity;

    Button start = activity.getMainMenu().getRouteStartButton();
    RoutingController.get().setStartButton(start);
    start.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        activity.closeMenu(Statistics.EventName.ROUTING_START, AlohaHelper.ROUTING_START, new Runnable()
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

  public void onSaveState(Bundle outState)
  {
    outState.putBoolean(STATE_OPEN, isOpen());
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, isAltitudeChartShown());
  }

  public void restoreState(Bundle state)
  {
    if (state.containsKey(STATE_OPEN))
      mSlotsRestoredState = state.getBoolean(STATE_OPEN);

    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showRouteAltitudeChart(!isVehicleRouteChecked());
  }

  @Override
  public void showRouteAltitudeChart(boolean show)
  {
    ImageView altitudeChart = (ImageView) mActivity.findViewById(R.id.altitude_chart);
    showRouteAltitudeChartInternal(show, altitudeChart);
  }

}

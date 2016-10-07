package com.mapswithme.maps.routing;

import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.widget.ImageView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  private static final String STATE_OPEN = "slots panel open";

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

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_OPEN, isOpen());
    saveRoutingPanelState(outState);
  }

  public void restoreState(@NonNull Bundle state)
  {
    if (state.containsKey(STATE_OPEN))
      mSlotsRestoredState = state.getBoolean(STATE_OPEN);

    restoreRoutingPanelState(state);
  }

  @Override
  public void showRouteAltitudeChart()
  {
    ImageView altitudeChart = (ImageView) mActivity.findViewById(R.id.altitude_chart);
    showRouteAltitudeChartInternal(altitudeChart);
  }
}

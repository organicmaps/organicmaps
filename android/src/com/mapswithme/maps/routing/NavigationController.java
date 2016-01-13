package com.mapswithme.maps.routing;

import android.app.Activity;
import android.location.Location;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.FlatProgressView;
import com.mapswithme.util.Animations;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class NavigationController
{
  private final View mFrame;

  private final TextView mDistanceTotal;
  private final TextView mTimeTotal;
  private final ImageView mTurnDirection;
  private final TextView mExitNumber;

  private final View mNextTurnFrame;
  private final ImageView mNextTurnImage;

  private final TextView mDistanceTurn;
  private final FlatProgressView mRouteProgress;
  private final TextView mNextStreet;
  private final TextView mTimeArrival;

  private double mNorth;

  public NavigationController(Activity activity)
  {
    mFrame = activity.findViewById(R.id.navigation_frame);

    mDistanceTotal = (TextView) mFrame.findViewById(R.id.tv__total_distance);
    mTimeTotal = (TextView) mFrame.findViewById(R.id.tv__total_time);
    mTimeArrival = (TextView) mFrame.findViewById(R.id.tv__arrival_time);
    mTurnDirection = (ImageView) mFrame.findViewById(R.id.iv__turn);
    mExitNumber = (TextView) mFrame.findViewById(R.id.tv__exit_num);

    mDistanceTurn = (TextView) mFrame.findViewById(R.id.tv__turn_distance);
    mRouteProgress = (FlatProgressView) mFrame.findViewById(R.id.fp__route_progress);
    mNextStreet = (TextView) mFrame.findViewById(R.id.tv__next_street);

    mFrame.findViewById(R.id.btn__close).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CLOSE);
        RoutingController.get().cancel();
      }
    });

    mNextTurnFrame = mFrame.findViewById(R.id.next_turn_frame);
    mNextTurnImage = (ImageView) mNextTurnFrame.findViewById(R.id.iv__next_turn);
  }

  private void updateVehicle(RoutingInfo info)
  {
    mDistanceTurn.setText(Utils.formatUnitsText(R.dimen.text_size_display_1, R.dimen.text_size_toolbar,
                                                info.distToTurn, info.turnUnits));
    info.vehicleTurnDirection.setTurnDrawable(mTurnDirection);
    if (RoutingInfo.VehicleTurnDirection.isRoundAbout(info.vehicleTurnDirection))
      UiUtils.setTextAndShow(mExitNumber, String.valueOf(info.exitNum));
    else
      UiUtils.hide(mExitNumber);

    if (info.vehicleNextTurnDirection.containsNextTurn())
    {
      Animations.appearSliding(mNextTurnFrame, Animations.TOP, null);
      info.vehicleNextTurnDirection.setNextTurnDrawable(mNextTurnImage);
    }
    else
      Animations.disappearSliding(mNextTurnFrame, Animations.BOTTOM, null);
  }

  private void updatePedestrian(RoutingInfo info)
  {
    Location next = info.pedestrianNextDirection;
    Location location = LocationHelper.INSTANCE.getLastLocation();
    DistanceAndAzimut da = Framework.nativeGetDistanceAndAzimutFromLatLon(next.getLatitude(), next.getLongitude(),
                                                                          location.getLatitude(), location.getLongitude(),
                                                                          mNorth);
    String[] splitDistance = da.getDistance().split(" ");
    mDistanceTurn.setText(Utils.formatUnitsText(R.dimen.text_size_display_1, R.dimen.text_size_toolbar,
                                                splitDistance[0], splitDistance[1]));
    if (info.pedestrianTurnDirection != null)
      RoutingInfo.PedestrianTurnDirection.setTurnDrawable(mTurnDirection, da);
  }

  public void updateNorth(double north)
  {
    if (!RoutingController.get().isNavigating())
      return;

    mNorth = north;
    update(Framework.nativeGetRouteFollowingInfo());
  }

  public void update(RoutingInfo info)
  {
    if (info == null)
      return;

    if (Framework.nativeGetRouter() == Framework.ROUTER_TYPE_VEHICLE)
      updateVehicle(info);
    else
      updatePedestrian(info);

    mTimeTotal.setText(RoutingController.formatRoutingTime(info.totalTimeInSeconds, R.dimen.text_size_routing_dimension));
    mDistanceTotal.setText(Utils.formatUnitsText(R.dimen.text_size_routing_number, R.dimen.text_size_routing_dimension,
                                                 info.distToTarget, info.targetUnits));
    mTimeArrival.setText(RoutingController.formatArrivalTime(info.totalTimeInSeconds));
    UiUtils.setTextAndHideIfEmpty(mNextStreet, info.nextStreet);
    mRouteProgress.setProgress((int) info.completionPercent);
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
    if (!show)
      UiUtils.hide(mNextTurnFrame);
  }
}

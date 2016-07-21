package com.mapswithme.maps.routing;

import android.app.Activity;
import android.location.Location;
import android.os.Build;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class NavigationController
{
  private final View mFrame;
  private final View mTopFrame;
  private final View mBottomFrame;

  private final ImageView mNextTurnImage;
  private final TextView mNextTurnDistance;
  private final TextView mCircleExit;

  private final View mNextNextTurnFrame;
  private final ImageView mNextNextTurnImage;

  private final View mStreetFrame;
  private final TextView mNextStreet;

//  private final TextView mDistanceTotal;
//  private final TextView mTimeTotal;
//  private final ImageView mTurnDirection;
//
//  private final FlatProgressView mRouteProgress;
//  private final TextView mTimeArrival;

  private double mNorth;

  public NavigationController(Activity activity)
  {
    mFrame = activity.findViewById(R.id.navigation_frame);
    mTopFrame = mFrame.findViewById(R.id.nav_top_frame);
    mBottomFrame = mFrame.findViewById(R.id.nav_bottom_frame);

    // Top frame
    View turnFrame = mTopFrame.findViewById(R.id.nav_next_turn_frame);
    mNextTurnImage = (ImageView) turnFrame.findViewById(R.id.turn);
    mNextTurnDistance = (TextView) turnFrame.findViewById(R.id.distance);
    mCircleExit = (TextView) turnFrame.findViewById(R.id.circle_exit);

    mNextNextTurnFrame = mTopFrame.findViewById(R.id.nav_next_next_turn_frame);
    mNextNextTurnImage = (ImageView) mNextNextTurnFrame.findViewById(R.id.turn);

    mStreetFrame = mTopFrame.findViewById(R.id.street_frame);
    mNextStreet = (TextView) mStreetFrame.findViewById(R.id.street);
    View shadow = mTopFrame.findViewById(R.id.shadow_top);
    UiUtils.showIf(Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP, shadow);

    // TODO (trashkalmar): Bottom frame
//    mDistanceTotal = (TextView) mFrame.findViewById(R.id.tv__total_distance);
//    mTimeTotal = (TextView) mFrame.findViewById(R.id.tv__total_time);
//    mTimeArrival = (TextView) mFrame.findViewById(R.id.tv__arrival_time);
//    mTurnDirection = (ImageView) mFrame.findViewById(R.id.iv__turn);
//
//    mRouteProgress = (FlatProgressView) mFrame.findViewById(R.id.fp__route_progress);
//
//    mFrame.findViewById(R.id.btn__close).setOnClickListener(new View.OnClickListener()
//    {
//      @Override
//      public void onClick(View v)
//      {
//        AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
//        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CLOSE);
//        RoutingController.get().cancel();
//      }
//    });
  }

  private void updateVehicle(RoutingInfo info)
  {
    mNextTurnDistance.setText(Utils.formatUnitsText(mFrame.getContext(),
                                                    R.dimen.text_size_nav_number,
                                                    R.dimen.text_size_nav_dimension,
                                                    info.distToTurn,
                                                    info.turnUnits));
    info.vehicleTurnDirection.setTurnDrawable(mNextTurnImage);
    if (RoutingInfo.VehicleTurnDirection.isRoundAbout(info.vehicleTurnDirection))
      UiUtils.setTextAndShow(mCircleExit, String.valueOf(info.exitNum));
    else
      UiUtils.hide(mCircleExit);

    UiUtils.showIf(info.vehicleNextTurnDirection.containsNextTurn(), mNextNextTurnFrame);
    if (info.vehicleNextTurnDirection.containsNextTurn())
      info.vehicleNextTurnDirection.setNextTurnDrawable(mNextNextTurnImage);
  }

  private void updatePedestrian(RoutingInfo info)
  {
    Location next = info.pedestrianNextDirection;
    Location location = LocationHelper.INSTANCE.getSavedLocation();
    DistanceAndAzimut da = Framework.nativeGetDistanceAndAzimuthFromLatLon(next.getLatitude(), next.getLongitude(),
                                                                           location.getLatitude(), location.getLongitude(),
                                                                           mNorth);
    String[] splitDistance = da.getDistance().split(" ");
    mNextTurnDistance.setText(Utils.formatUnitsText(mFrame.getContext(),
                                                    R.dimen.text_size_nav_number,
                                                    R.dimen.text_size_nav_dimension,
                                                    splitDistance[0],
                                                    splitDistance[1]));
    if (info.pedestrianTurnDirection != null)
      RoutingInfo.PedestrianTurnDirection.setTurnDrawable(mNextTurnImage, da);
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

    if (Framework.nativeGetRouter() == Framework.ROUTER_TYPE_PEDESTRIAN)
      updatePedestrian(info);
    else
      updateVehicle(info);

    boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    UiUtils.showIf(hasStreet, mStreetFrame);
    if (!TextUtils.isEmpty(info.nextStreet))
      mNextStreet.setText(info.nextStreet);

    /*
    mTimeTotal.setText(RoutingController.formatRoutingTime(mFrame.getContext(),
                                                           info.totalTimeInSeconds,
                                                           R.dimen.text_size_routing_dimension));
    mDistanceTotal.setText(Utils.formatUnitsText(mFrame.getContext(),
                                                 R.dimen.text_size_routing_number,
                                                 R.dimen.text_size_routing_dimension,
                                                 info.distToTarget,
                                                 info.targetUnits));
    mTimeArrival.setText(RoutingController.formatArrivalTime(info.totalTimeInSeconds));
    mRouteProgress.setProgress((int) info.completionPercent);*/
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
  }
}

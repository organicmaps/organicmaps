package com.mapswithme.maps.routing;

import android.app.Activity;
import android.content.Intent;
import android.location.Location;
import android.os.Build;
import android.text.TextUtils;
import android.util.Pair;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.concurrent.TimeUnit;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.FlatProgressView;
import com.mapswithme.maps.widget.menu.NavMenu;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class NavigationController
{
  private final View mFrame;
  private final View mTopFrame;
  private final View mBottomFrame;
  private final NavMenu mNavMenu;

  private final ImageView mNextTurnImage;
  private final TextView mNextTurnDistance;
  private final TextView mCircleExit;

  private final View mNextNextTurnFrame;
  private final ImageView mNextNextTurnImage;

  private final View mStreetFrame;
  private final TextView mNextStreet;

  private final TextView mSpeedValue;
  private final TextView mSpeedUnits;
  private final TextView mTimeHourValue;
  private final TextView mTimeHourUnits;
  private final TextView mTimeMinuteValue;
  private final TextView mTimeMinuteUnits;
  private final TextView mDistanceValue;
  private final TextView mDistanceUnits;
  private final FlatProgressView mRouteProgress;

  private double mNorth;

  public NavigationController(Activity activity)
  {
    mFrame = activity.findViewById(R.id.navigation_frame);
    mTopFrame = mFrame.findViewById(R.id.nav_top_frame);
    mBottomFrame = mFrame.findViewById(R.id.nav_bottom_frame);
    mNavMenu = createNavMenu();
    mNavMenu.refreshTts();

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

    // Bottom frame
    mSpeedValue = (TextView) mBottomFrame.findViewById(R.id.speed_value);
    mSpeedUnits = (TextView) mBottomFrame.findViewById(R.id.speed_dimen);
    mTimeHourValue = (TextView) mBottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = (TextView) mBottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = (TextView) mBottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = (TextView) mBottomFrame.findViewById(R.id.time_minute_dimen);
    mDistanceValue = (TextView) mBottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = (TextView) mBottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = (FlatProgressView) mBottomFrame.findViewById(R.id.navigation_progress);
  }

  private NavMenu createNavMenu()
  {
    return new NavMenu(mBottomFrame, new NavMenu.ItemClickListener<NavMenu.Item>()
    {
      @Override
      public void onItemClick(NavMenu.Item item)
      {
        switch (item)
        {
        case STOP:
          RoutingController.get().cancel();
          Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CLOSE);
          AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
          break;
        case SETTINGS:
          final MwmActivity parent = ((MwmActivity) mFrame.getContext());
          parent.closeMenu(Statistics.EventName.ROUTING_SETTINGS, AlohaHelper.MENU_SETTINGS, new Runnable()
          {
            @Override
            public void run()
            {
              parent.startActivity(new Intent(parent, SettingsActivity.class));
            }
          });
          break;
        case TTS_VOLUME:
          TtsPlayer.setEnabled(!TtsPlayer.isEnabled());
          mNavMenu.refreshTts();
          Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CLOSE);
          AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
          break;
        }
      }
    });
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

    final Location last = LocationHelper.INSTANCE.getLastKnownLocation();
    if (last != null)
    {
      Pair<String, String> speedAndUnits = StringUtils.nativeFormatSpeedAndUnits(last.getSpeed());
      mSpeedValue.setText(speedAndUnits.first);
      mSpeedUnits.setText(speedAndUnits.second);
    }
    updateTime(info.totalTimeInSeconds);
    mDistanceValue.setText(info.distToTarget);
    mDistanceUnits.setText(info.targetUnits);
    mRouteProgress.setProgress((int) info.completionPercent);
  }

  private void updateTime(int seconds)
  {
    final long hours = TimeUnit.SECONDS.toHours(seconds);
    final long minutes = TimeUnit.MINUTES.toMinutes(seconds) % 60;
    mTimeMinuteValue.setText(String.valueOf(minutes));
    // TODO set localized text
    mTimeMinuteUnits.setText("m");
    if (hours == 0)
    {
      UiUtils.hide(mTimeHourUnits, mTimeHourValue);
      return;
    }
    mTimeHourValue.setText(String.valueOf(hours));
    // TODO set localized text
    mTimeHourUnits.setText("h");
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
    mNavMenu.show(show);
  }

  public NavMenu getNavMenu()
  {
    return mNavMenu;
  }
}

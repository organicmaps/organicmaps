package com.mapswithme.maps.routing;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MediaPlayerWrapper;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.FlatProgressView;
import com.mapswithme.maps.widget.menu.NavMenu;
import com.mapswithme.util.Animations;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_BOOKMARKS_CLICK;

public class NavigationController implements TrafficManager.TrafficCallback, View.OnClickListener
{
  private static final String STATE_SHOW_TIME_LEFT = "ShowTimeLeft";

  private final View mFrame;
  private final View mBottomFrame;
  private final View mSearchButtonFrame;
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
  private final ImageView mDotTimeLeft;
  private final ImageView mDotTimeArrival;
  private final TextView mDistanceValue;
  private final TextView mDistanceUnits;
  private final FlatProgressView mRouteProgress;

  @NonNull
  private final SearchWheel mSearchWheel;
  @NonNull
  private final View mSpeedViewContainer;

  private boolean mShowTimeLeft = true;

  private double mNorth;

  @NonNull
  private final MediaPlayer.OnCompletionListener mSpeedCamSignalCompletionListener;

  public NavigationController(Activity activity)
  {
    mFrame = activity.findViewById(R.id.navigation_frame);
    mBottomFrame = mFrame.findViewById(R.id.nav_bottom_frame);
    mBottomFrame.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        switchTimeFormat();
      }
    });
    mNavMenu = createNavMenu();
    mNavMenu.refresh();

    // Top frame
    View topFrame = mFrame.findViewById(R.id.nav_top_frame);
    View turnFrame = topFrame.findViewById(R.id.nav_next_turn_frame);
    mNextTurnImage = (ImageView) turnFrame.findViewById(R.id.turn);
    mNextTurnDistance = (TextView) turnFrame.findViewById(R.id.distance);
    mCircleExit = (TextView) turnFrame.findViewById(R.id.circle_exit);

    mNextNextTurnFrame = topFrame.findViewById(R.id.nav_next_next_turn_frame);
    mNextNextTurnImage = (ImageView) mNextNextTurnFrame.findViewById(R.id.turn);

    mStreetFrame = topFrame.findViewById(R.id.street_frame);
    mNextStreet = (TextView) mStreetFrame.findViewById(R.id.street);
    View shadow = topFrame.findViewById(R.id.shadow_top);
    UiUtils.showIf(Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP, shadow);

    UiUtils.extendViewWithStatusBar(mStreetFrame);
    UiUtils.extendViewMarginWithStatusBar(turnFrame);

    // Bottom frame
    mSpeedViewContainer = mBottomFrame.findViewById(R.id.speed_view_container);
    mSpeedValue = (TextView) mBottomFrame.findViewById(R.id.speed_value);
    mSpeedUnits = (TextView) mBottomFrame.findViewById(R.id.speed_dimen);
    mTimeHourValue = (TextView) mBottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = (TextView) mBottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = (TextView) mBottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = (TextView) mBottomFrame.findViewById(R.id.time_minute_dimen);
    mDotTimeArrival = (ImageView) mBottomFrame.findViewById(R.id.dot_estimate);
    mDotTimeLeft = (ImageView) mBottomFrame.findViewById(R.id.dot_left);
    mDistanceValue = (TextView) mBottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = (TextView) mBottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = (FlatProgressView) mBottomFrame.findViewById(R.id.navigation_progress);

    mSearchButtonFrame = activity.findViewById(R.id.search_button_frame);
    mSearchWheel = new SearchWheel(mSearchButtonFrame);

    ImageView bookmarkButton = (ImageView) mSearchButtonFrame.findViewById(R.id.btn_bookmarks);
    bookmarkButton.setImageDrawable(Graphics.tint(bookmarkButton.getContext(),
                                                  R.drawable.ic_menu_bookmarks));
    bookmarkButton.setOnClickListener(this);
    Application app = (Application) bookmarkButton.getContext().getApplicationContext();
    mSpeedCamSignalCompletionListener = new CameraWarningSignalCompletionListener(app);
  }

  public void onResume()
  {
    mNavMenu.onResume(null);
    mSearchWheel.onResume();
  }

  public boolean performSearchClick()
  {
    return mSearchWheel.performClick();
  }

  private NavMenu createNavMenu()
  {
    return new NavMenu(mBottomFrame, this::onMenuItemClicked);
  }

  private void onMenuItemClicked(NavMenu.Item item)
  {
    final MwmActivity parent = ((MwmActivity) mFrame.getContext());
    switch (item)
    {
    case STOP:
      mNavMenu.close(false);
      Statistics.INSTANCE.trackRoutingFinish(true,
                                             RoutingController.get().getLastRouterType(),
                                             TrafficManager.INSTANCE.isEnabled());
      RoutingController.get().cancel();
      break;
    case SETTINGS:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_SETTINGS);
      parent.closeMenu(() -> parent.startActivity(new Intent(parent, SettingsActivity.class)));
      break;
    case TTS_VOLUME:
      TtsPlayer.setEnabled(!TtsPlayer.isEnabled());
      mNavMenu.refreshTts();
      break;
    case TRAFFIC:
      TrafficManager.INSTANCE.toggle();
      parent.onTrafficLayerSelected();
      mNavMenu.refreshTraffic();
      //TODO: Add statistics reporting (in separate task)
      break;
    case TOGGLE:
      mNavMenu.toggle(true);
      parent.refreshFade();
    }
  }

  public void stop(MwmActivity parent)
  {
    parent.refreshFade();
    mSearchWheel.reset();
  }

  private void updateVehicle(RoutingInfo info)
  {
    mNextTurnDistance.setText(Utils.formatUnitsText(mFrame.getContext(),
                                                    R.dimen.text_size_nav_number,
                                                    R.dimen.text_size_nav_dimension,
                                                    info.distToTurn,
                                                    info.turnUnits));
    info.carDirection.setTurnDrawable(mNextTurnImage);
    if (RoutingInfo.CarDirection.isRoundAbout(info.carDirection))
      UiUtils.setTextAndShow(mCircleExit, String.valueOf(info.exitNum));
    else
      UiUtils.hide(mCircleExit);

    UiUtils.showIf(info.nextCarDirection.containsNextTurn(), mNextNextTurnFrame);
    if (info.nextCarDirection.containsNextTurn())
      info.nextCarDirection.setNextTurnDrawable(mNextNextTurnImage);
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

  public void update(@Nullable RoutingInfo info)
  {
    if (info == null)
      return;

    if (Framework.nativeGetRouter() == Framework.ROUTER_TYPE_PEDESTRIAN)
      updatePedestrian(info);
    else
      updateVehicle(info);

    updateStreetView(info);
    updateSpeedView(info);
    updateTime(info.totalTimeInSeconds);
    mDistanceValue.setText(info.distToTarget);
    mDistanceUnits.setText(info.targetUnits);
    mRouteProgress.setProgress((int) info.completionPercent);
    playbackSpeedCamWarning(info);
  }

  private void updateStreetView(@NonNull RoutingInfo info)
  {
    boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    UiUtils.showIf(hasStreet, mStreetFrame);
    if (!TextUtils.isEmpty(info.nextStreet))
      mNextStreet.setText(info.nextStreet);
  }

  private void updateSpeedView(@NonNull RoutingInfo info)
  {
    final Location last = LocationHelper.INSTANCE.getLastKnownLocation();
    if (last == null)
      return;

    Pair<String, String> speedAndUnits = StringUtils.nativeFormatSpeedAndUnits(last.getSpeed());

    mSpeedUnits.setText(speedAndUnits.second);
    mSpeedValue.setText(speedAndUnits.first);
    mSpeedViewContainer.setActivated(info.isSpeedLimitExceeded());
  }

  private void playbackSpeedCamWarning(@NonNull RoutingInfo info)
  {
    if (!info.shouldPlayWarningSignal() || TtsPlayer.INSTANCE.isSpeaking())
      return;

    Context context = mBottomFrame.getContext();
    MediaPlayerWrapper player = MediaPlayerWrapper.from(context);
    player.playback(R.raw.speed_cams_beep, mSpeedCamSignalCompletionListener);
  }

  private void updateTime(int seconds)
  {
    if (mShowTimeLeft)
      updateTimeLeft(seconds);
    else
      updateTimeEstimate(seconds);

    mDotTimeLeft.setEnabled(mShowTimeLeft);
    mDotTimeArrival.setEnabled(!mShowTimeLeft);
  }

  private void updateTimeLeft(int seconds)
  {
    final long hours = TimeUnit.SECONDS.toHours(seconds);
    final long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    UiUtils.setTextAndShow(mTimeMinuteValue, String.valueOf(minutes));
    String min = mFrame.getResources().getString(R.string.minute);
    UiUtils.setTextAndShow(mTimeMinuteUnits, min);
    if (hours == 0)
    {
      UiUtils.hide(mTimeHourUnits, mTimeHourValue);
      return;
    }
    UiUtils.setTextAndShow(mTimeHourValue, String.valueOf(hours));
    String hour = mFrame.getResources().getString(R.string.hour);
    UiUtils.setTextAndShow(mTimeHourUnits, hour);
  }

  private void updateTimeEstimate(int seconds)
  {
    final Calendar currentTime = Calendar.getInstance();
    currentTime.add(Calendar.SECOND, seconds);
    final DateFormat timeFormat12 = new SimpleDateFormat("h:mm aa", Locale.getDefault());
    final DateFormat timeFormat24 = new SimpleDateFormat("HH:mm", Locale.getDefault());
    boolean is24Format = android.text.format.DateFormat.is24HourFormat(mTimeMinuteValue.getContext());
    UiUtils.setTextAndShow(mTimeMinuteValue, is24Format ? timeFormat24.format(currentTime.getTime())
                          : timeFormat12.format(currentTime.getTime()));
    UiUtils.hide(mTimeHourUnits, mTimeHourValue, mTimeMinuteUnits);
  }

  private void switchTimeFormat()
  {
    mShowTimeLeft = !mShowTimeLeft;
    update(Framework.nativeGetRouteFollowingInfo());
  }

  public void showSearchButtons(boolean show)
  {
    UiUtils.showIf(show, mSearchButtonFrame);
  }

  public void adjustSearchButtons(int width)
  {
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mSearchButtonFrame.getLayoutParams();
    params.setMargins(width, params.topMargin, params.rightMargin, params.bottomMargin);
    mSearchButtonFrame.requestLayout();
  }

  public void updateSearchButtonsTranslation(float translation)
  {
    mSearchButtonFrame.setTranslationY(translation);
  }

  public void fadeInSearchButtons()
  {
    UiUtils.show(mSearchButtonFrame);
    Animations.fadeInView(mSearchButtonFrame, null);
  }

  public void fadeOutSearchButtons()
  {
    Animations.fadeOutView(mSearchButtonFrame, new Runnable()
    {
      @Override
      public void run()
      {
        mSearchButtonFrame.setVisibility(View.INVISIBLE);
      }
    });
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
    UiUtils.showIf(show, mSearchButtonFrame);
    mNavMenu.show(show);
  }

  public void resetSearchWheel()
  {
    mSearchWheel.reset();
  }

  public NavMenu getNavMenu()
  {
    return mNavMenu;
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_SHOW_TIME_LEFT, mShowTimeLeft);
    mSearchWheel.saveState(outState);
  }

  public void onRestoreState(@NonNull Bundle savedInstanceState)
  {
    mShowTimeLeft = savedInstanceState.getBoolean(STATE_SHOW_TIME_LEFT);
    mSearchWheel.restoreState(savedInstanceState);
  }

  @Override
  public void onEnabled()
  {
    mNavMenu.refreshTraffic();
  }

  @Override
  public void onDisabled()
  {
    mNavMenu.refreshTraffic();
  }

  @Override
  public void onWaitingData()
  {
    // no op
  }

  @Override
  public void onOutdated()
  {
    // no op
  }

  @Override
  public void onNoData()
  {
    // no op
  }

  @Override
  public void onNetworkError()
  {
    // no op
  }

  @Override
  public void onExpiredData()
  {
    // no op
  }

  @Override
  public void onExpiredApp()
  {
    // no op
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
      case R.id.btn_bookmarks:
        BookmarkCategoriesActivity.start(mFrame.getContext());
        Statistics.INSTANCE.trackRoutingEvent(ROUTING_BOOKMARKS_CLICK,
                                              RoutingController.get().isPlanning());
        break;
    }
  }

  public void destroy()
  {
    MediaPlayerWrapper.from(mBottomFrame.getContext()).release();
  }

  private static class CameraWarningSignalCompletionListener implements MediaPlayer.OnCompletionListener
  {
    @NonNull
    private final Application mApp;

    CameraWarningSignalCompletionListener(@NonNull Application app)
    {
      mApp = app;
    }

    @Override
    public void onCompletion(MediaPlayer mp)
    {
      TtsPlayer.INSTANCE.playTurnNotifications(mApp);
    }
  }
}

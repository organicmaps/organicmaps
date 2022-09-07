package com.mapswithme.maps.routing;

import android.app.Activity;
import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.IBinder;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MediaPlayerWrapper;
import com.mapswithme.maps.maplayer.MapButtonsController;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.menu.NavMenu;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class NavigationController implements Application.ActivityLifecycleCallbacks,
                                             TrafficManager.TrafficCallback,
                                             NavMenu.NavMenuListener
{
  private static final String STATE_BOUND = "Bound";

  private final View mFrame;

  private final ImageView mNextTurnImage;
  private final TextView mNextTurnDistance;
  private final TextView mCircleExit;

  private final View mNextNextTurnFrame;
  private final ImageView mNextNextTurnImage;

  private final View mStreetFrame;
  private final TextView mNextStreet;

  @NonNull
  private final MapButtonsController mMapButtonsController;

  @NonNull
  private final MediaPlayer.OnCompletionListener mSpeedCamSignalCompletionListener;

  private final NavMenu mNavMenu;
  View.OnClickListener mOnSettingsClickListener;
  @Nullable
  private NavigationService mService = null;
  private boolean mBound = false;
  @NonNull
  private final ServiceConnection mServiceConnection = new ServiceConnection()
  {
    @Override
    public void onServiceConnected(ComponentName name, IBinder service)
    {
      NavigationService.LocalBinder binder = (NavigationService.LocalBinder) service;
      mService = binder.getService();
      mBound = true;
      doBackground();
    }

    @Override
    public void onServiceDisconnected(ComponentName name)
    {
      mService = null;
      mBound = false;
    }
  };

  public NavigationController(AppCompatActivity activity, @NonNull MapButtonsController mapButtonsController, View.OnClickListener onSettingsClickListener)
  {
    mFrame = activity.findViewById(R.id.navigation_frame);
    mNavMenu = new NavMenu(activity, this);
    mOnSettingsClickListener = onSettingsClickListener;
    mMapButtonsController = mapButtonsController;

    // Show a blank view below the navbar to hide the menu content
    mFrame.findViewById(R.id.nav_bottom_sheet_nav_bar).setOnApplyWindowInsetsListener((view, windowInsets) -> {
      view.getLayoutParams().height = windowInsets.getSystemWindowInsetBottom();
      view.getLayoutParams().width = mFrame.findViewById(R.id.nav_bottom_sheet).getWidth();
      return windowInsets;
    });

    // Top frame
    View topFrame = mFrame.findViewById(R.id.nav_top_frame);
    View turnFrame = topFrame.findViewById(R.id.nav_next_turn_frame);
    mNextTurnImage = turnFrame.findViewById(R.id.turn);
    mNextTurnDistance = turnFrame.findViewById(R.id.distance);
    mCircleExit = turnFrame.findViewById(R.id.circle_exit);

    topFrame.findViewById(R.id.nav_next_turn_container).setOnApplyWindowInsetsListener((view, windowInsets) -> {
      view.setPadding(windowInsets.getSystemWindowInsetLeft(), view.getPaddingTop(),
                      view.getPaddingRight(), view.getPaddingBottom());
      return windowInsets;
    });

    mNextNextTurnFrame = topFrame.findViewById(R.id.nav_next_next_turn_frame);
    mNextNextTurnImage = mNextNextTurnFrame.findViewById(R.id.turn);

    mStreetFrame = topFrame.findViewById(R.id.street_frame);
    mNextStreet = mStreetFrame.findViewById(R.id.street);
    View shadow = topFrame.findViewById(R.id.shadow_top);
    UiUtils.hide(shadow);

    UiUtils.extendViewWithStatusBar(mStreetFrame);
    UiUtils.extendViewMarginWithStatusBar(turnFrame);

    final Application app = (Application) mFrame.getContext().getApplicationContext();
    mSpeedCamSignalCompletionListener = new CameraWarningSignalCompletionListener(app);
  }

  public void stop(MwmActivity parent)
  {
    mMapButtonsController.resetSearch();

    if (mBound)
    {
      parent.unbindService(mServiceConnection);
      mBound = false;
      if (mService != null)
        mService.stopSelf();
    }
  }

  public void start(@NonNull MwmActivity parent)
  {
    parent.bindService(new Intent(parent, NavigationService.class),
                       mServiceConnection,
                       Context.BIND_AUTO_CREATE);
    mBound = true;
    parent.startService(new Intent(parent, NavigationService.class));
  }

  public void doForeground()
  {
    if (mService != null)
      mService.doForeground();
  }

  public void doBackground()
  {
    if (mService != null)
      mService.stopForeground(true);
  }

  private void updateVehicle(RoutingInfo info)
  {
    if (!TextUtils.isEmpty(info.distToTurn))
    {
      SpannableStringBuilder nextTurnDistance = Utils.formatUnitsText(mFrame.getContext(),
                                                                      R.dimen.text_size_nav_number,
                                                                      R.dimen.text_size_nav_dimension,
                                                                      info.distToTurn,
                                                                      info.turnUnits);
      mNextTurnDistance.setText(nextTurnDistance);
      info.carDirection.setTurnDrawable(mNextTurnImage);
    }

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
    mNextTurnDistance.setText(
        Utils.formatUnitsText(mFrame.getContext(), R.dimen.text_size_nav_number,
                              R.dimen.text_size_nav_dimension, info.distToTurn, info.turnUnits));

    info.pedestrianTurnDirection.setTurnDrawable(mNextTurnImage);
  }

  public void updateNorth()
  {
    if (!RoutingController.get().isNavigating())
      return;

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
    mNavMenu.update(info);
    playbackSpeedCamWarning(info);
  }

  private void updateStreetView(@NonNull RoutingInfo info)
  {
    boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    UiUtils.showIf(hasStreet, mStreetFrame);
    if (!TextUtils.isEmpty(info.nextStreet))
      mNextStreet.setText(info.nextStreet);
  }

  private void playbackSpeedCamWarning(@NonNull RoutingInfo info)
  {
    if (!info.shouldPlayWarningSignal() || TtsPlayer.INSTANCE.isSpeaking())
      return;

    Context context = mFrame.getContext();
    MediaPlayerWrapper player = MediaPlayerWrapper.from(context);
    player.playback(R.raw.speed_cams_beep, mSpeedCamSignalCompletionListener);
  }

  public void show(boolean show)
  {
    if (show && !UiUtils.isVisible(mFrame))
      collapseNavMenu();
    else if (!show && UiUtils.isVisible(mFrame))
      mNavMenu.hideNavBottomSheet();
    UiUtils.showIf(show, mFrame);
  }

  public boolean isNavMenuCollapsed()
  {
    return mNavMenu.getBottomSheetState() == BottomSheetBehavior.STATE_COLLAPSED;
  }

  public boolean isNavMenuHidden()
  {
    return mNavMenu.getBottomSheetState() == BottomSheetBehavior.STATE_HIDDEN;
  }

  public void collapseNavMenu()
  {
    mNavMenu.collapseNavBottomSheet();
  }

  @Override
  public void onActivityCreated(@NonNull Activity activity, @Nullable Bundle savedInstanceState)
  {
    // no op
  }

  @Override
  public void onActivityStarted(@NonNull Activity activity)
  {
    // no op
  }

  @Override
  public void onActivityResumed(@NonNull Activity activity)
  {
    mNavMenu.refreshTts();
    if (mBound)
      doBackground();
  }

  @Override
  public void onActivityPaused(Activity activity)
  {
    doForeground();
  }

  @Override
  public void onActivityStopped(@NonNull Activity activity)
  {
    // no op
  }

  @Override
  public void onActivitySaveInstanceState(@NonNull Activity activity, @NonNull Bundle outState)
  {
    outState.putBoolean(STATE_BOUND, mBound);
    mMapButtonsController.saveNavSearchState(outState);
  }

  public void onRestoreState(@NonNull Bundle savedInstanceState, @NonNull MwmActivity parent)
  {
    mBound = savedInstanceState.getBoolean(STATE_BOUND);
    if (mBound)
      start(parent);
    mMapButtonsController.restoreNavSearchState(savedInstanceState);
  }

  @Override
  public void onActivityDestroyed(@NonNull Activity activity)
  {
    // no op
  }

  @Override
  public void onEnabled()
  {
    // mNavMenu.refreshTraffic();
  }

  @Override
  public void onDisabled()
  {
    // mNavMenu.refreshTraffic();
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

  public void destroy()
  {
    MediaPlayerWrapper.from(mFrame.getContext()).release();
  }

  @Override
  public void onSettingsClicked()
  {
    mOnSettingsClickListener.onClick(null);
  }

  @Override
  public void onStopClicked()
  {
    mNavMenu.hideNavBottomSheet();
    RoutingController.get().cancel();
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

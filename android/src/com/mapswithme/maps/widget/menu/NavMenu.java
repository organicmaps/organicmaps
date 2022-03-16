package com.mapswithme.maps.widget.menu;

import android.animation.ValueAnimator;
import android.location.Location;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.IntegerRes;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.FlatProgressView;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

public class NavMenu
{
  private final BottomSheetBehavior<View> mNavBottomSheetBehavior;

  private final ImageView mTts;
  private final ImageView mToggle;
  private final View mSpeedViewContainer;
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

  @IntegerRes
  private final int mAnimationDuration;
  private final AppCompatActivity mActivity;
  private final NavMenuListener mNavMenuListener;
  private boolean mShowTimeLeft = true;

  public NavMenu(AppCompatActivity activity, NavMenuListener navMenuListener)
  {
    mActivity = activity;
    mNavMenuListener = navMenuListener;
    mAnimationDuration = MwmApplication.from(mActivity).getResources().getInteger(R.integer.anim_menu);
    mNavBottomSheetBehavior = BottomSheetBehavior.from(mActivity.findViewById(R.id.nav_bottom_sheet));
    BottomSheetBehavior.BottomSheetCallback bottomSheetCallback = new BottomSheetBehavior.BottomSheetCallback()
    {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (newState == BottomSheetBehavior.STATE_EXPANDED)
          setToggleState(true, true);
        else if (newState == BottomSheetBehavior.STATE_COLLAPSED)
          setToggleState(false, true);
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {

      }
    };
    mNavBottomSheetBehavior.addBottomSheetCallback(bottomSheetCallback);

    View mBottomFrame = mActivity.findViewById(R.id.nav_bottom_frame);
    mBottomFrame.setOnClickListener(v -> switchTimeFormat());

    // Bottom frame
    mSpeedViewContainer = mBottomFrame.findViewById(R.id.speed_view_container);
    mSpeedValue = mBottomFrame.findViewById(R.id.speed_value);
    mSpeedUnits = mBottomFrame.findViewById(R.id.speed_dimen);
    mTimeHourValue = mBottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = mBottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = mBottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = mBottomFrame.findViewById(R.id.time_minute_dimen);
    mDotTimeArrival = mBottomFrame.findViewById(R.id.dot_estimate);
    mDotTimeLeft = mBottomFrame.findViewById(R.id.dot_left);
    mDistanceValue = mBottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = mBottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = mBottomFrame.findViewById(R.id.navigation_progress);

    // Bottom frame buttons
    mToggle = mBottomFrame.findViewById(R.id.toggle);
    mToggle.setImageDrawable(Graphics.tint(mActivity, R.drawable.ic_menu_close, R.attr.iconTintLight));
    mToggle.setOnClickListener(v -> onToggleClicked());
    ImageView mSettings = mBottomFrame.findViewById(R.id.settings);
    mSettings.setOnClickListener(v -> onSettingsClicked());
    mTts = mBottomFrame.findViewById(R.id.tts_volume);
    mTts.setOnClickListener(v -> onTtsClicked());
    Button stop = mBottomFrame.findViewById(R.id.stop);
    stop.setOnClickListener(v -> onStopClicked());
    UiUtils.updateRedButton(stop);

    hideNavBottomSheet();
  }

  private void onStopClicked()
  {
    mNavMenuListener.onStopClicked();
  }

  private void onSettingsClicked()
  {
    mNavMenuListener.onSettingsClicked();
  }

  private void onTtsClicked()
  {
    TtsPlayer.setEnabled(!TtsPlayer.isEnabled());
    refreshTts();
  }

  private void onToggleClicked()
  {
    toggleNavBottomSheet();
  }


  private void switchTimeFormat()
  {
    mShowTimeLeft = !mShowTimeLeft;
    mNavMenuListener.onNavMenuUpdate();
  }

  public void collapseNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
  }

  public void expandNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  public void toggleNavBottomSheet()
  {
    if (mNavBottomSheetBehavior.getState() == BottomSheetBehavior.STATE_COLLAPSED)
      expandNavBottomSheet();
    else
      collapseNavBottomSheet();
  }

  public void hideNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    setToggleState(false, false);
  }

  protected void setToggleState(boolean open, boolean animate)
  {
    final float to = open ? -90.0f : 90.0f;
    if (!animate)
    {
      mToggle.setRotation(to);
      mToggle.invalidate();
      return;
    }

    final float from = -to;
    ValueAnimator animator = ValueAnimator.ofFloat(from, to);
    animator.addUpdateListener(animation -> mToggle.setRotation((float) animation.getAnimatedValue()));

    animator.setDuration(mAnimationDuration);
    animator.start();
  }

  public void refreshTts()
  {
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mActivity, R.drawable.ic_voice_on,
        R.attr.colorAccent)
        : Graphics.tint(mActivity, R.drawable.ic_voice_off));
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
    String min = mActivity.getResources().getString(R.string.minute);
    UiUtils.setTextAndShow(mTimeMinuteUnits, min);
    if (hours == 0)
    {
      UiUtils.hide(mTimeHourUnits, mTimeHourValue);
      return;
    }
    UiUtils.setTextAndShow(mTimeHourValue, String.valueOf(hours));
    String hour = mActivity.getResources().getString(R.string.hour);
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


  private void updateSpeedView(@NonNull RoutingInfo info)
  {
    final Location last = LocationHelper.INSTANCE.getSavedLocation();
    if (last == null)
      return;

    Pair<String, String> speedAndUnits = StringUtils.nativeFormatSpeedAndUnits(last.getSpeed());

    mSpeedUnits.setText(speedAndUnits.second);
    mSpeedValue.setText(speedAndUnits.first);
    mSpeedViewContainer.setActivated(info.isSpeedLimitExceeded());
  }

  public void update(@NonNull RoutingInfo info)
  {
    updateSpeedView(info);
    updateTime(info.totalTimeInSeconds);
    mDistanceValue.setText(info.distToTarget);
    mDistanceUnits.setText(info.targetUnits);
    mRouteProgress.setProgress((int) info.completionPercent);
  }

  public boolean isShowTimeLeft()
  {
    return mShowTimeLeft;
  }

  public void setShowTimeLeft(boolean showTimeLeft)
  {
    this.mShowTimeLeft = showTimeLeft;
  }

  public interface NavMenuListener
  {
    void onStopClicked();

    void onSettingsClicked();

    void onNavMenuUpdate();
  }
}

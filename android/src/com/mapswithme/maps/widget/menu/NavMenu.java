package com.mapswithme.maps.widget.menu;

import android.location.Location;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
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
  private final View mBottomSheetBackground;
  private final View mHeaderFrame;

  private final ImageView mTts;
  private final View mSpeedViewContainer;
  private final TextView mSpeedValue;
  private final TextView mSpeedUnits;
  private final TextView mTimeHourValue;
  private final TextView mTimeHourUnits;
  private final TextView mTimeMinuteValue;
  private final TextView mTimeMinuteUnits;
  private final TextView mTimeEstimate;
  private final TextView mDistanceValue;
  private final TextView mDistanceUnits;
  private final FlatProgressView mRouteProgress;

  private final AppCompatActivity mActivity;
  private final NavMenuListener mNavMenuListener;

  private int currentPeekHeight = 0;

  public NavMenu(AppCompatActivity activity, NavMenuListener navMenuListener)
  {
    mActivity = activity;
    mNavMenuListener = navMenuListener;
    View mBottomFrame = mActivity.findViewById(R.id.nav_bottom_frame);
    mHeaderFrame = mBottomFrame.findViewById(R.id.line_frame);
    mHeaderFrame.setOnClickListener(v -> toggleNavMenu());
    mHeaderFrame.addOnLayoutChangeListener((view, i, i1, i2, i3, i4, i5, i6, i7) -> setPeekHeight());
    mNavBottomSheetBehavior = BottomSheetBehavior.from(mActivity.findViewById(R.id.nav_bottom_sheet));
    mBottomSheetBackground = mActivity.findViewById(R.id.nav_bottom_sheet_background);
    mBottomSheetBackground.setOnClickListener(v -> collapseNavBottomSheet());
    mBottomSheetBackground.setVisibility(View.GONE);
    mBottomSheetBackground.setAlpha(0);
    mNavBottomSheetBehavior.addBottomSheetCallback(new BottomSheetBehavior.BottomSheetCallback()
    {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (newState == BottomSheetBehavior.STATE_COLLAPSED || newState == BottomSheetBehavior.STATE_HIDDEN)
        {
          mBottomSheetBackground.setVisibility(View.GONE);
          mBottomSheetBackground.setAlpha(0);
        } else
        {
          mBottomSheetBackground.setVisibility(View.VISIBLE);
        }
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {
        mBottomSheetBackground.setAlpha(slideOffset);
      }
    });

    // Bottom frame
    mSpeedViewContainer = mBottomFrame.findViewById(R.id.speed_view_container);
    mSpeedValue = mBottomFrame.findViewById(R.id.speed_value);
    mSpeedUnits = mBottomFrame.findViewById(R.id.speed_dimen);
    mTimeHourValue = mBottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = mBottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = mBottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = mBottomFrame.findViewById(R.id.time_minute_dimen);
    mTimeEstimate = mBottomFrame.findViewById(R.id.time_estimate);
    mDistanceValue = mBottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = mBottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = mBottomFrame.findViewById(R.id.navigation_progress);

    // Bottom frame buttons
    ImageView mSettings = mBottomFrame.findViewById(R.id.settings);
    mSettings.setOnClickListener(v -> onSettingsClicked());
    mTts = mBottomFrame.findViewById(R.id.tts_volume);
    mTts.setOnClickListener(v -> onTtsClicked());
    Button stop = mBottomFrame.findViewById(R.id.stop);
    stop.setOnClickListener(v -> onStopClicked());
    UiUtils.updateRedButton(stop);
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

  private void toggleNavMenu()
  {
    if (getBottomSheetState() == BottomSheetBehavior.STATE_EXPANDED)
      collapseNavBottomSheet();
    else
      expandNavBottomSheet();
  }

  public void setPeekHeight()
  {
    int headerHeight = mHeaderFrame.getHeight();
    if (currentPeekHeight != headerHeight)
    {
      currentPeekHeight = headerHeight;
      mNavBottomSheetBehavior.setPeekHeight(currentPeekHeight);
    }
  }

  public void collapseNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
  }

  public void expandNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  public int getBottomSheetState()
  {
    return mNavBottomSheetBehavior.getState();
  }

  public void refreshTts()
  {
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mActivity, R.drawable.ic_voice_on,
        R.attr.colorAccent)
        : Graphics.tint(mActivity, R.drawable.ic_voice_off));
  }


  private void updateTime(int seconds)
  {
    updateTimeLeft(seconds);
    updateTimeEstimate(seconds);
  }

  private void updateTimeLeft(int seconds)
  {
    final long hours = TimeUnit.SECONDS.toHours(seconds);
    final long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    mTimeMinuteValue.setText(String.valueOf(minutes));
    String min = mActivity.getResources().getString(R.string.minute);
    mTimeMinuteUnits.setText(min);
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
    DateFormat timeFormat;
    if (android.text.format.DateFormat.is24HourFormat(mTimeMinuteValue.getContext()))
      timeFormat = new SimpleDateFormat("HH:mm", Locale.getDefault());
    else
      timeFormat = new SimpleDateFormat("h:mm aa", Locale.getDefault());
    mTimeEstimate.setText(timeFormat.format(currentTime.getTime()));
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

  public interface NavMenuListener
  {
    void onStopClicked();

    void onSettingsClicked();
  }
}

package app.organicmaps.widget.menu;

import android.location.Location;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import app.organicmaps.R;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.routing.RoutingInfo;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.FlatProgressView;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

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
  private final TextView mTimeHourValue2;
  private final TextView mTimeHourUnits2;
  private final TextView mTimeMinuteValue2;
  private final TextView mTimeMinuteUnits2;
  private final TextView mTimeEstimate;
  private final TextView mTimeEstimate2;
  private final TextView mDistanceValue;
  private final TextView mDistanceUnits;
  private final FlatProgressView mRouteProgress;
  private final AppCompatActivity mActivity;
  private final NavMenuListener mNavMenuListener;
  private LinearLayout mTimeValueContainer;
  private LinearLayout mTimeValueContainer2;

  private int currentPeekHeight = 0;

  public NavMenu(AppCompatActivity activity, NavMenuListener navMenuListener)
  {
    mActivity = activity;
    mNavMenuListener = navMenuListener;
    final View bottomFrame = mActivity.findViewById(R.id.nav_bottom_frame);
    mHeaderFrame = bottomFrame.findViewById(R.id.line_frame);
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
    mSpeedViewContainer = bottomFrame.findViewById(R.id.speed_view_container);
    mSpeedValue = bottomFrame.findViewById(R.id.speed_value);
    mSpeedUnits = bottomFrame.findViewById(R.id.speed_dimen);
    mTimeHourValue = bottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = bottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = bottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = bottomFrame.findViewById(R.id.time_minute_dimen);
    mTimeHourValue2 = bottomFrame.findViewById(R.id.time_hour_value2);
    mTimeHourUnits2 = bottomFrame.findViewById(R.id.time_hour_dimen2);
    mTimeMinuteValue2 = bottomFrame.findViewById(R.id.time_minute_value2);
    mTimeMinuteUnits2 = bottomFrame.findViewById(R.id.time_minute_dimen2);
    mTimeEstimate = bottomFrame.findViewById(R.id.time_estimate);
    mTimeEstimate2 = bottomFrame.findViewById(R.id.time_estimate2);
    mDistanceValue = bottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = bottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = bottomFrame.findViewById(R.id.navigation_progress);
    mTimeValueContainer = bottomFrame.findViewById(R.id.time_values_container);
    mTimeValueContainer2 = bottomFrame.findViewById(R.id.time_values_container2);

    // Bottom frame buttons
    ImageView mSettings = bottomFrame.findViewById(R.id.settings);
    mSettings.setOnClickListener(v -> onSettingsClicked());
    mTts = bottomFrame.findViewById(R.id.tts_volume);
    mTts.setOnClickListener(v -> onTtsClicked());
    Button stop = bottomFrame.findViewById(R.id.stop);
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
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mActivity,
        R.drawable.ic_voice_on, R.attr.colorAccent)
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
    mTimeMinuteValue2.setText(String.valueOf(minutes));
    String min = mActivity.getResources().getString(R.string.minute);
    mTimeMinuteUnits.setText(min);
    mTimeMinuteUnits2.setText(min);
    mTimeValueContainer.setOnClickListener(v -> changeTimes());
    if (hours == 0)
    {
      UiUtils.hide(mTimeHourUnits, mTimeHourValue);
      UiUtils.hide(mTimeHourUnits2, mTimeHourValue2);
      return;
    }
    UiUtils.setTextAndShow(mTimeHourValue, String.valueOf(hours));
    UiUtils.setTextAndShow(mTimeHourValue2, String.valueOf(hours));
    String hour = mActivity.getResources().getString(R.string.hour);
    UiUtils.setTextAndShow(mTimeHourUnits, hour);
    UiUtils.setTextAndShow(mTimeHourUnits2, hour);
  }

  public void changeTimes()
  {
    if (mTimeValueContainer.getVisibility() == View.VISIBLE)
    {
      UiUtils.hide(mTimeValueContainer);
      UiUtils.show(mTimeValueContainer2);
      UiUtils.hide(mTimeEstimate);
      UiUtils.show(mTimeEstimate2);
    }
    else
    {
      UiUtils.show(mTimeValueContainer);
      UiUtils.hide(mTimeValueContainer2);
      UiUtils.show(mTimeEstimate);
      UiUtils.hide(mTimeEstimate2);
    }
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
    mTimeEstimate2.setText(timeFormat.format(currentTime.getTime()));
    mTimeEstimate2.setOnClickListener(v -> changeTimes());
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
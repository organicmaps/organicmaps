package app.organicmaps.maplayer;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.RelativeSizeSpan;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.OptIn;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.routing.RoutingPlanViewModel;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.downloader.UpdateInfo;
import app.organicmaps.sdk.location.NoGps;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.sdk.maplayer.isolines.IsolinesManager;
import app.organicmaps.sdk.maplayer.subway.SubwayManager;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.search.SearchPageViewModel;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;
import app.organicmaps.widget.menu.MyPositionButton;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.badge.BadgeDrawable;
import com.google.android.material.badge.BadgeUtils;
import com.google.android.material.badge.ExperimentalBadgeUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import java.util.HashMap;
import java.util.Map;

public class MapButtonsController extends Fragment
{
  Map<MapButtons, View> mButtonsMap;
  private View mFrame;
  private View mInnerLeftButtonsFrame;
  private View mInnerRightButtonsFrame;
  @Nullable
  private View mBottomButtonsFrame;
  @Nullable
  private LayersButton mToggleMapLayerButton;
  @Nullable
  FloatingActionButton mTrackRecordingStatusButton;
  @Nullable
  private MyPositionButton mNavMyPosition;
  @Nullable
  private View mCarControls;
  @Nullable
  private FloatingActionButton mManualPositionButton;
  @Nullable
  private View mReverseButton;
  @Nullable
  private TextView mPositionStatus;
  @Nullable
  private FloatingActionButton mPauseButton;
  // The color of the icon of the pause button as its style sets it.
  @Nullable
  private ColorStateList mPauseButtonIconTint;
  @Nullable
  private View mShiftPositionContainer;
  @Nullable
  private TextView mShiftPositionStep;
  @Nullable
  private View mShiftPositionForward;
  @Nullable
  private View mShiftPositionBack;
  private static final long POSITION_STATUS_UPDATE_INTERVAL_MS = 1000;
  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Runnable mPositionStatusUpdater = new Runnable() {
    @Override
    public void run()
    {
      updatePositionStatus();
      mHandler.postDelayed(this, POSITION_STATUS_UPDATE_INTERVAL_MS);
    }
  };
  private SearchWheel mSearchWheel;
  private BadgeDrawable mBadgeDrawable;
  @Nullable
  private ObjectAnimator mBlinkingAnimator;
  private float mContentHeight;
  private float mContentWidth;

  private MapButtonClickListener mMapButtonClickListener;
  private PlacePageViewModel mPlacePageViewModel;
  private RoutingPlanViewModel mRoutingPlanViewModel;
  private MapButtonsViewModel mMapButtonsViewModel;
  private SearchPageViewModel mSearchPageViewModel;

  private final Observer<Integer> mPlacePageDistanceToTopObserver = translationY -> move(translationY, true);
  private final Observer<Integer> mRoutingBottomDistanceToTopObserver = translationY -> move(translationY, false);
  private final Observer<Boolean> mBottomButtonHiddenObserver = this::setBottomButtonsHidden;
  private final Observer<Integer> mSearchPageDistanceToTopObserver = this::moveForSearch;
  private final Observer<Boolean> mButtonHiddenObserver = this::setButtonsHidden;
  private final Observer<Integer> mMyPositionModeObserver = this::updateNavMyPositionButton;
  private final Observer<SearchWheel.SearchOption> mSearchOptionObserver = this::onSearchOptionChange;
  private final Observer<Boolean> mTrackRecorderObserver = (enable) ->
  {
    updateMenuBadge(enable);
    showButton(enable, MapButtons.trackRecordingStatus);
  };
  private final Observer<Integer> mTopButtonMarginObserver = this::updateTopButtonsMargin;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    final FragmentActivity activity = requireActivity();
    mMapButtonClickListener = (MwmActivity) activity;
    mRoutingPlanViewModel = new ViewModelProvider(activity).get(RoutingPlanViewModel.class);
    mPlacePageViewModel = new ViewModelProvider(activity).get(PlacePageViewModel.class);
    mMapButtonsViewModel = new ViewModelProvider(activity).get(MapButtonsViewModel.class);
    mSearchPageViewModel = new ViewModelProvider(activity).get(SearchPageViewModel.class);
    if (mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation)
      mFrame = inflater.inflate(R.layout.map_buttons_layout_navigation, container, false);
    else
      mFrame = inflater.inflate(R.layout.map_buttons_layout_regular, container, false);

    mInnerLeftButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_left);
    mInnerRightButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_right);
    mBottomButtonsFrame = mFrame.findViewById(R.id.map_buttons_bottom);

    final FloatingActionButton helpButton = mFrame.findViewById(R.id.help_button);
    final View zoomFrame = mFrame.findViewById(R.id.zoom_buttons_container);
    mFrame.findViewById(R.id.nav_zoom_in)
        .setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.zoomIn));
    mFrame.findViewById(R.id.nav_zoom_out)
        .setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.zoomOut));
    final View bookmarksButton = mFrame.findViewById(R.id.btn_bookmarks);
    bookmarksButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.bookmarks));
    final View myPosition = mFrame.findViewById(R.id.my_position);
    mNavMyPosition =
        new MyPositionButton(myPosition, (v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.myPosition));
    mCarControls = mFrame.findViewById(R.id.car_controls);
    mManualPositionButton = mFrame.findViewById(R.id.manual_position);
    mPositionStatus = mFrame.findViewById(R.id.position_status);
    mPositionStatus.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.positionStatus));
    mManualPositionButton.setOnClickListener(
        (v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.manualPosition));
    mShiftPositionContainer = mFrame.findViewById(R.id.shift_position_container);
    mShiftPositionStep = mFrame.findViewById(R.id.shift_position_step);
    mShiftPositionForward = mFrame.findViewById(R.id.shift_position_forward);
    mShiftPositionForward.setOnClickListener(
        (v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.shiftPositionForward));
    mShiftPositionBack = mFrame.findViewById(R.id.shift_position_back);
    mShiftPositionBack.setOnClickListener(
        (v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.shiftPositionBack));
    mReverseButton = mFrame.findViewById(R.id.reverse_direction);
    mReverseButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.reverseDirection));
    mPauseButton = mFrame.findViewById(R.id.pause_movement);
    mPauseButtonIconTint = mPauseButton.getImageTintList();
    mPauseButton.setOnClickListener((v) -> {
      mMapButtonClickListener.onMapButtonClick(MapButtons.pauseMovement);
      updatePositionStatus();
    });
    mShiftPositionStep.setOnClickListener((v) -> {
      NoGps.nativeCycleShiftStep();
      updatePositionStatus();
    });

    // Some buttons do not exist in navigation mode
    mToggleMapLayerButton = mFrame.findViewById(R.id.layers_button);
    if (mToggleMapLayerButton != null)
    {
      mToggleMapLayerButton.setOnClickListener(
          view -> mMapButtonClickListener.onMapButtonClick(MapButtons.toggleMapLayer));
      mToggleMapLayerButton.setVisibility(View.VISIBLE);
    }
    // The navigation panel keeps its margin: it is set on a position update only, and the buttons are recreated
    // when the navigation starts, maybe without a position.
    if (mMapButtonsViewModel.getLayoutMode().getValue() != LayoutMode.navigation)
      mMapButtonsViewModel.setTopButtonsMarginTop(-1);
    mTrackRecordingStatusButton = mFrame.findViewById(R.id.track_recording_status);
    if (mTrackRecordingStatusButton != null)
      mTrackRecordingStatusButton.setOnClickListener(
          view -> mMapButtonClickListener.onMapButtonClick(MapButtons.trackRecordingStatus));
    final View menuButton = mFrame.findViewById(R.id.menu_button);
    if (menuButton != null)
    {
      menuButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.menu));
      // This hack is needed to show the badge on the initial startup. For some reason, updateMenuBadge does not work
      // from onResume() there.
      menuButton.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
        @Override
        public void onGlobalLayout()
        {
          updateMenuBadge();
          menuButton.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        }
      });
    }
    if (helpButton != null)
      helpButton.setOnClickListener((v) -> mMapButtonClickListener.onMapButtonClick(MapButtons.help));

    mSearchWheel =
        new SearchWheel(mFrame,
                        (v)
                            -> mMapButtonClickListener.onMapButtonClick(MapButtons.search),
                        (v) -> mMapButtonClickListener.onSearchCanceled(), mMapButtonsViewModel, mSearchPageViewModel);
    final View searchButton = mFrame.findViewById(R.id.btn_search);

    // Used to get the maximum height the buttons will evolve in
    mFrame.addOnLayoutChangeListener(new MapButtonsController.ContentViewLayoutChangeListener(mFrame));

    mButtonsMap = new HashMap<>();
    mButtonsMap.put(MapButtons.zoom, zoomFrame);
    mButtonsMap.put(MapButtons.myPosition, myPosition);
    mButtonsMap.put(MapButtons.bookmarks, bookmarksButton);
    mButtonsMap.put(MapButtons.search, searchButton);

    if (mToggleMapLayerButton != null)
      mButtonsMap.put(MapButtons.toggleMapLayer, mToggleMapLayerButton);
    if (menuButton != null)
      mButtonsMap.put(MapButtons.menu, menuButton);
    if (helpButton != null)
      mButtonsMap.put(MapButtons.help, helpButton);
    if (mTrackRecordingStatusButton != null)
      mButtonsMap.put(MapButtons.trackRecordingStatus, mTrackRecordingStatusButton);
    showButton(false, MapButtons.trackRecordingStatus);
    return mFrame;
  }
  // For disabling bottom buttons which are visible in tablets
  private void setBottomButtonsHidden(boolean hide)
  {
    if (mBottomButtonsFrame != null)
      UiUtils.showIf(!hide, mBottomButtonsFrame);
  }

  public void showButton(boolean show, MapButtonsController.MapButtons button)
  {
    // TODO(AB): Why do we need this check? Isn't it better to crash and fix the wrong logic ASAP?
    final View buttonView = mButtonsMap.get(button);
    if (buttonView == null)
      return;
    switch (button)
    {
    case zoom: UiUtils.showIf(show && Config.showZoomButtons(), buttonView); break;
    case toggleMapLayer:
      if (mToggleMapLayerButton != null)
        UiUtils.showIf(show && !isInNavigationMode(), mToggleMapLayerButton);
      break;
    case myPosition:
      if (mNavMyPosition != null)
        mNavMyPosition.showButton(show);
      if (mCarControls != null)
        UiUtils.showIf(show, mCarControls);
      break;
    case search: mSearchWheel.show(show);
    case bookmarks:
    case menu: UiUtils.showIf(show, buttonView); break;
    case trackRecordingStatus:
      UiUtils.showIf(show, buttonView);
      animateIconBlinking(show, (FloatingActionButton) buttonView);
    }
  }

  /**
   * Shows where the current position comes from: GPS, cell towers, the user (manual mode) or nowhere.
   */
  private void updatePositionStatus()
  {
    final Context context = requireContext();
    final NoGps.Status status = NoGps.getStatus();
    final NoGps.PositionSource source = status.getSource();

    if (mPositionStatus != null)
    {
      final String text;
      final int color;
      switch (source)
      {
      case GPS ->
      {
        text = getString(R.string.nogps_status_gps);
        color = R.color.nogps_status_gps;
      }
      case NETWORK ->
      {
        final String accuracy = formatAccuracy(status.accuracyM);
        text = getString(
            status.gpsSpoofed ? R.string.nogps_status_network_spoofed : R.string.nogps_status_network_no_gps, accuracy);
        color = R.color.nogps_status_network;
      }
      case INERTIAL ->
      {
        text = status.paused ? getString(R.string.nogps_status_paused)
                             : getString(R.string.nogps_status_inertial, formatAccuracy(status.accuracyM));
        color = R.color.nogps_status_inertial;
      }
      case MANUAL ->
      {
        final long minutes = status.manualAgeMs / 60_000;
        text = minutes == 0 ? getString(R.string.nogps_status_manual_just_now)
                            : getString(R.string.nogps_status_manual_minutes, minutes);
        color = R.color.nogps_status_manual;
      }
      default ->
      {
        text = getString(R.string.nogps_status_none);
        color = R.color.nogps_status_none;
      }
      }
      // The position is not taken from GPS or towers now, but the user has to see whether they work: in the
      // manual mode GPS is not used even when it is back.
      if (status.manualMode || source == NoGps.PositionSource.INERTIAL)
      {
        final SpannableString withOthers = new SpannableString(text + "\n" + getOtherSourcesStatus(status));
        withOthers.setSpan(new RelativeSizeSpan(0.8f), text.length() + 1, withOthers.length(),
                           Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mPositionStatus.setText(withOthers);
      }
      else
        mPositionStatus.setText(text);
      mPositionStatus.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(context, color)));
    }

    // The buttons moving the position along the road calibrate the speed, they are shown while the user calibrates
    // it.
    if (mShiftPositionContainer != null)
      UiUtils.showIf(status.movedByHand && status.shiftButtonsShown, mShiftPositionContainer);
    if (mShiftPositionStep != null)
      mShiftPositionStep.setText(getString(R.string.nogps_meters, status.shiftStepM));
    if (mReverseButton != null)
      UiUtils.showIf(status.movedByHand, mReverseButton);

    // The movement is paused only when it is calculated from the car speed.
    if (mPauseButton != null)
    {
      UiUtils.showIf(status.movedByHand && status.inertialEnabled, mPauseButton);
      mPauseButton.setImageResource(status.paused ? R.drawable.ic_play : R.drawable.ic_pause);
      mPauseButton.setContentDescription(getString(status.paused ? R.string.nogps_resume : R.string.nogps_pause));
      // The pause set by the navigation itself is to be noticed: the mark waits until the car drives along a
      // road again or the user resumes the movement.
      final boolean byItself = status.paused && status.pausedByItself;
      // The style tints the background by the attribute of the view, not of the button: the tint of the button
      // is not seen under it.
      final Drawable background = mPauseButton.getBackground();
      if (background != null)
        background.setTint(byItself ? ContextCompat.getColor(context, R.color.nogps_paused_by_itself)
                                    : ThemeUtils.getColor(context, R.attr.menuBackground));
      mPauseButton.setImageTintList(byItself ? ColorStateList.valueOf(Color.WHITE) : mPauseButtonIconTint);
    }

    // The position waits at a turn until the car leaves it, otherwise it would go to another street.
    setShiftEnabled(mShiftPositionForward, !status.shiftForwardBlocked);
    setShiftEnabled(mShiftPositionBack, !status.shiftBackBlocked);

    // The icon tells where the position comes from now: from satellites or from the user.
    if (mManualPositionButton != null)
    {
      mManualPositionButton.setImageResource(status.manualMode ? R.drawable.ic_manual_position
                                                               : R.drawable.ic_gps_position);
    }
  }

  private static void setShiftEnabled(@Nullable View button, boolean enabled)
  {
    if (button == null)
      return;
    button.setEnabled(enabled);
    button.setAlpha(enabled ? 1f : 0.4f);
  }

  @NonNull
  private String getOtherSourcesStatus(@NonNull NoGps.Status status)
  {
    if (status.workingGpsAccuracyM >= 0)
      return getString(R.string.nogps_status_gps_back, formatAccuracy(status.workingGpsAccuracyM));

    final String gpsStatus =
        getString(status.gpsSpoofed ? R.string.nogps_status_gps_spoofed : R.string.nogps_status_no_gps);
    if (status.workingNetworkAccuracyM < 0)
      return gpsStatus;
    return gpsStatus + " · " + getString(R.string.nogps_status_towers, formatAccuracy(status.workingNetworkAccuracyM));
  }

  @NonNull
  private String formatAccuracy(float meters)
  {
    if (meters < 1000)
      return getString(R.string.nogps_meters, Math.round(meters));
    return getString(R.string.nogps_kilometers, meters / 1000);
  }

  void animateIconBlinking(boolean show, @NonNull FloatingActionButton button)
  {
    if (mBlinkingAnimator != null)
    {
      mBlinkingAnimator.cancel();
      mBlinkingAnimator = null;
    }
    if (show)
    {
      Drawable drawable = button.getDrawable();
      mBlinkingAnimator = ObjectAnimator.ofArgb(drawable, "tint", 0xFF757575, 0xFFFF0000);
      mBlinkingAnimator.setDuration(2500);
      mBlinkingAnimator.setEvaluator(new ArgbEvaluator());
      mBlinkingAnimator.setRepeatCount(ObjectAnimator.INFINITE);
      mBlinkingAnimator.setRepeatMode(ObjectAnimator.REVERSE);
      mBlinkingAnimator.start();
    }
  }

  private static int dpToPx(float dp, Context context)
  {
    return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, context.getResources().getDisplayMetrics());
  }

  private void updateTopButtonsMargin(int margin)
  {
    if (margin == -1)
      return;
    // The navigation panel at the top must not cover these views.
    for (View view : new View[] {mTrackRecordingStatusButton, mPositionStatus})
    {
      if (view == null)
        continue;
      final ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
      params.topMargin = margin;
      view.setLayoutParams(params);
    }
  }

  @OptIn(markerClass = ExperimentalBadgeUtils.class)
  private void updateMenuBadge(Boolean enable)
  {
    final View menuButton = mButtonsMap.get(MapButtons.menu);
    final Context context = getContext();
    // Sometimes the global layout listener fires when the fragment is not attached to a context
    if (menuButton == null || context == null)
      return;
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final int verticalOffset = dpToPx(8, context) + dpToPx(Integer.toString(0).length() * 5, context);

    if (count == 0)
    {
      BadgeUtils.detachBadgeDrawable(mBadgeDrawable, menuButton);
      mBadgeDrawable = BadgeDrawable.create(context);
      mBadgeDrawable.setMaxCharacterCount(0);
      mBadgeDrawable.setHorizontalOffset(verticalOffset);
      mBadgeDrawable.setVerticalOffset(dpToPx(9, context));
      mBadgeDrawable.setBackgroundColor(getResources().getColor(R.color.base_accent));
      mBadgeDrawable.setVisible(enable);
      BadgeUtils.attachBadgeDrawable(mBadgeDrawable, menuButton);
    }
  }

  @OptIn(markerClass = com.google.android.material.badge.ExperimentalBadgeUtils.class)
  public void updateMenuBadge()
  {
    final View menuButton = mButtonsMap.get(MapButtons.menu);
    final Context context = getContext();
    // Sometimes the global layout listener fires when the fragment is not attached to a context
    if (menuButton == null || context == null)
      return;
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final int verticalOffset = dpToPx(8, context) + dpToPx(Integer.toString(0).length() * 5, context);
    BadgeUtils.detachBadgeDrawable(mBadgeDrawable, menuButton);
    mBadgeDrawable = BadgeDrawable.create(context);
    mBadgeDrawable.setMaxCharacterCount(3);
    mBadgeDrawable.setHorizontalOffset(verticalOffset);
    mBadgeDrawable.setVerticalOffset(dpToPx(9, context));
    mBadgeDrawable.setNumber(count);
    mBadgeDrawable.setVisible(count > 0);
    BadgeUtils.attachBadgeDrawable(mBadgeDrawable, menuButton);

    updateMenuBadge(TrackRecorder.nativeIsTrackRecordingEnabled());
  }

  public void updateHelpButtonIcon()
  {
    final View view = mButtonsMap.get(MapButtons.help);
    if (!(view instanceof FloatingActionButton helpButton))
      return;

    if (Framework.nativeCanShowCrowdfundingPromo() && !TextUtils.isEmpty(Utils.getDonateUrl(requireContext())))
    {
      helpButton.setImageResource(R.drawable.ic_crowdfunding);
      helpButton.getDrawable().setTintList(null);
    }
    else if (Config.isNY() && !TextUtils.isEmpty(Utils.getDonateUrl(requireContext())))
    {
      helpButton.setImageResource(R.drawable.ic_christmas_tree);
      helpButton.getDrawable().setTintList(null);
    }
    else
    {
      helpButton.setImageResource(app.organicmaps.branding.R.drawable.logo);
      // Keep this button colorful in normal theme.
      if (!ThemeUtils.isDarkTheme(requireContext()))
        helpButton.getDrawable().setTintList(null);
    }
  }

  public void updateLayerButton()
  {
    if (mToggleMapLayerButton == null)
      return;
    final boolean buttonSelected = TrafficManager.INSTANCE.isEnabled() || IsolinesManager.isEnabled()
                                || SubwayManager.isEnabled() || Framework.nativeIsOutdoorsLayerEnabled()
                                || Framework.nativeIsHikingLayerEnabled() || Framework.nativeIsCyclingLayerEnabled()
                                || Framework.nativeIsBackgroundTilesEnabled();
    mToggleMapLayerButton.setHasActiveLayers(buttonSelected);
  }

  private boolean isBehindPlacePage(View v)
  {
    if (mPlacePageViewModel == null)
      return false;
    final Integer placePageWidth = mPlacePageViewModel.getPlacePageWidth().getValue();
    if (placePageWidth != null)
      return !(mContentWidth / 2 > (placePageWidth.floatValue() / 2.0) + v.getWidth());
    return true;
  }

  private boolean isBehindSearchSheet(View v)
  {
    if (mSearchPageViewModel == null)
      return false;
    final Integer searchPageWidth = mSearchPageViewModel.getSearchPageWidth().getValue();
    if (searchPageWidth != null)
      return !(mContentWidth / 2 > (searchPageWidth.floatValue() / 2.0) + v.getWidth());
    return true;
  }

  private boolean isMoving(View v)
  {
    return v.getTranslationY() < 0;
  }

  public void move(float translationY, boolean shouldActivate)
  {
    if (RoutingController.get().isNavigating() || mContentHeight == 0)
      return;
    final boolean pp = Boolean.TRUE.equals(mRoutingPlanViewModel.getIsPlacePageActive().getValue());
    // don't apply move in landscape
    if (!shouldActivate == pp || getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
      return;
    if (mInnerRightButtonsFrame != null)
      applyMove(mInnerRightButtonsFrame, translationY);
  }

  private void moveForSearch(float translationY)
  {
    if (mContentHeight == 0)
      return;

    if (mInnerRightButtonsFrame != null
        && (isBehindSearchSheet(mInnerRightButtonsFrame) || isMoving(mInnerRightButtonsFrame)))
      applyMove(mInnerRightButtonsFrame, translationY);
    if (mInnerLeftButtonsFrame != null
        && (isBehindSearchSheet(mInnerLeftButtonsFrame) || isMoving(mInnerLeftButtonsFrame)))
      applyMove(mInnerLeftButtonsFrame, translationY);
  }

  private void applyMove(View frame, float translationY)
  {
    final float rightTranslation = translationY - frame.getBottom();
    final float appliedTranslation = rightTranslation <= 0 ? rightTranslation : 0;
    frame.setTranslationY(appliedTranslation);
    updateButtonsVisibility(appliedTranslation, frame);
  }

  public void updateButtonsVisibility()
  {
    if (mInnerLeftButtonsFrame != null)
      updateButtonsVisibility(mInnerLeftButtonsFrame.getTranslationY(), mInnerLeftButtonsFrame);
    if (mInnerRightButtonsFrame != null)
      updateButtonsVisibility(mInnerRightButtonsFrame.getTranslationY(), mInnerRightButtonsFrame);
  }

  private void updateButtonsVisibility(final float translation, @Nullable View parent)
  {
    if (parent == null)
      return;
    for (Map.Entry<MapButtons, View> entry : mButtonsMap.entrySet())
    {
      final View button = entry.getValue();
      if (button.getParent() == parent)
      {
        int toleranceOffset = 0;
        // Allow offset tolerance for zoom buttons
        switch (entry.getKey())
        {
        case zoomIn:
        case zoomOut:
        case zoom: toleranceOffset = -140; break;
        }
        showButton(getViewTopOffset(translation, button) >= toleranceOffset, entry.getKey());
      }
    }
  }

  private float getBottomButtonsHeight()
  {
    if (mBottomButtonsFrame != null && mFrame != null && UiUtils.isVisible(mFrame))
      return mBottomButtonsFrame.getMeasuredHeight();
    else
      return 0;
  }

  public void setButtonsHidden(boolean buttonHidden)
  {
    UiUtils.showIf(!buttonHidden, mFrame);
    if (!buttonHidden)
      updateButtonsVisibility();
    mMapButtonsViewModel.setBottomButtonsHeight(getBottomButtonsHeight());
  }

  private boolean isInNavigationMode()
  {
    return RoutingController.get().isPlanning() || RoutingController.get().isNavigating();
  }

  public void updateNavMyPositionButton(int newMode)
  {
    if (mNavMyPosition != null)
      mNavMyPosition.update(newMode);
  }

  private int getViewTopOffset(float translation, View v)
  {
    return (int) (translation + v.getTop());
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    // FragmentStateManager requests insets for the frame before onViewCreated(), but the dispatch
    // itself only happens on the next layout pass — so a listener attached here still receives it.
    // Attaching in onResume() is too late: the dispatch has already run and nothing re-requests
    // insets for an already attached view, leaving the padding at zero.
    ViewCompat.setOnApplyWindowInsetsListener(
        view, WindowInsetUtils.PaddingInsetsListener.allSides(WindowInsetsCompat.Type.systemBars()
                                                              | WindowInsetsCompat.Type.displayCutout()));
  }

  @Override
  public void onStart()
  {
    super.onStart();
    final var viewLifecycleOwner = getViewLifecycleOwner();
    mRoutingPlanViewModel.getRoutingBottomDistanceToTop().observe(viewLifecycleOwner,
                                                                  mRoutingBottomDistanceToTopObserver);
    mPlacePageViewModel.getPlacePageDistanceToTop().observe(viewLifecycleOwner, mPlacePageDistanceToTopObserver);
    mMapButtonsViewModel.getBottomButtonsHidden().observe(viewLifecycleOwner, mBottomButtonHiddenObserver);
    mMapButtonsViewModel.getButtonsHidden().observe(viewLifecycleOwner, mButtonHiddenObserver);
    mSearchPageViewModel.getSearchPageDistanceToTop().observe(viewLifecycleOwner, mSearchPageDistanceToTopObserver);
    mMapButtonsViewModel.getMyPositionMode().observe(viewLifecycleOwner, mMyPositionModeObserver);
    mMapButtonsViewModel.getSearchOption().observe(viewLifecycleOwner, mSearchOptionObserver);
    mMapButtonsViewModel.getTrackRecorderState().observe(viewLifecycleOwner, mTrackRecorderObserver);
    mMapButtonsViewModel.getTopButtonsMarginTop().observe(viewLifecycleOwner, mTopButtonMarginObserver);
    mHandler.post(mPositionStatusUpdater);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation)
      mSearchWheel.onResume();
    updateMenuBadge();
    updateLayerButton();
    updateHelpButtonIcon();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mHandler.removeCallbacks(mPositionStatusUpdater);
    if (mBlinkingAnimator != null)
    {
      mBlinkingAnimator.cancel();
      mBlinkingAnimator = null;
    }
  }

  public void onSearchOptionChange(@Nullable SearchWheel.SearchOption searchOption)
  {
    if (searchOption == null && mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation)
      mSearchWheel.reset();
  }

  public enum LayoutMode
  {
    regular,
    planning,
    navigation
  }

  public enum MapButtons
  {
    myPosition,
    toggleMapLayer,
    zoomIn,
    zoomOut,
    zoom,
    search,
    bookmarks,
    menu,
    help,
    trackRecordingStatus,
    manualPosition,
    positionStatus,
    shiftPositionForward,
    shiftPositionBack,
    reverseDirection,
    pauseMovement
  }

  public interface MapButtonClickListener
  {
    void onMapButtonClick(MapButtons button);

    void onSearchCanceled();
  }

  private class ContentViewLayoutChangeListener implements View.OnLayoutChangeListener
  {
    @NonNull
    private final View mContentView;

    public ContentViewLayoutChangeListener(@NonNull View contentView)
    {
      mContentView = contentView;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                               int oldBottom)
    {
      mContentHeight = bottom - top;
      mContentWidth = right - left;
      mMapButtonsViewModel.setBottomButtonsHeight(getBottomButtonsHeight());
      mContentView.removeOnLayoutChangeListener(this);
    }
  }
}

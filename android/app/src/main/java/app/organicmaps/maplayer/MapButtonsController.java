package app.organicmaps.maplayer;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.OptIn;
import androidx.constraintlayout.widget.ConstraintLayout;
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
  private SearchOptionsButton mSearchOptionsButton;
  private BadgeDrawable mBadgeDrawable;
  @Nullable
  private ObjectAnimator mBlinkingAnimator;
  private float mContentHeight;
  private float mContentWidth;
  private boolean mIsNavSideColumn;
  private boolean mLeftButtonsAbovePanel;
  @NonNull
  private NavColumnMetrics mNavColumnMetrics = new NavColumnMetrics(0, 0, 0);

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
  private final Observer<SearchOptionsButton.SearchOption> mSearchOptionObserver = this::onSearchOptionChange;
  private final Observer<Boolean> mTrackRecorderObserver = (enable) ->
  {
    updateMenuBadge(enable);
    showButton(enable, MapButtons.trackRecordingStatus);
  };
  private final Observer<Integer> mTopButtonMarginObserver = this::updateTopButtonsMargin;
  private final Observer<NavColumnMetrics> mNavColumnMetricsObserver = this::updateLeftButtonsPlacement;

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

    // The navigation layouts park the left buttons beside the maneuver card wherever the nav panel
    // is the fixed-width start column. Same R.bool every other consumer of that branch reads
    // (MwmActivity, PlacePageUtils, NavigationController), so a tablet in portrait - a start column
    // too, unlike a phone - is not mistaken for the full-width arrangement.
    mIsNavSideColumn = mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation
                    && !getResources().getBoolean(R.bool.nav_full_width_card);
    // Window insets land on mFrame as padding after the first layout pass, so re-apply once the
    // container settles instead of keeping a stale bottom margin.
    if (mIsNavSideColumn && mInnerLeftButtonsFrame != null)
      mInnerLeftButtonsFrame.addOnLayoutChangeListener(
          (v, l, t, r, b, oldL, oldT, oldR, oldB) -> updateLeftButtonsPlacement(mNavColumnMetrics));

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

    // Some buttons do not exist in navigation mode
    mToggleMapLayerButton = mFrame.findViewById(R.id.layers_button);
    if (mToggleMapLayerButton != null)
    {
      mToggleMapLayerButton.setOnClickListener(
          view -> mMapButtonClickListener.onMapButtonClick(MapButtons.toggleMapLayer));
      mToggleMapLayerButton.setVisibility(View.VISIBLE);
    }
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

    mSearchOptionsButton = new SearchOptionsButton(
        mFrame,
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
      break;
    case search: mSearchOptionsButton.show(show);
    case bookmarks:
    case menu: UiUtils.showIf(show, buttonView); break;
    case trackRecordingStatus:
      UiUtils.showIf(show, buttonView);
      animateIconBlinking(show, (FloatingActionButton) buttonView);
    }
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
    if (margin == -1 || mTrackRecordingStatusButton == null)
      return;
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mTrackRecordingStatusButton.getLayoutParams();
    params.topMargin = margin;
    mTrackRecordingStatusButton.setLayoutParams(params);
  }

  // Start-column navigation (landscape phone, tablets): keep the bookmarks and search buttons in
  // the column, in a row right above the ETA panel, and stack them beside the maneuver card while
  // the column is too short.
  private void updateLeftButtonsPlacement(@NonNull NavColumnMetrics metrics)
  {
    mNavColumnMetrics = metrics;
    if (!mIsNavSideColumn || mInnerLeftButtonsFrame == null)
      return;

    // Height of the row the buttons would form above the panel - one button plus the container
    // padding. Taken from the dimens rather than measured, because the container currently holds
    // the other arrangement.
    final int padding = getResources().getDimensionPixelSize(R.dimen.nav_frame_padding);
    final int rowHeight = getResources().getDimensionPixelSize(R.dimen.map_button_size) + 2 * padding;
    // The card height drifts between maneuvers, so require some slack before moving into the
    // column and let the buttons stay there until they really stop fitting.
    final boolean abovePanel = metrics.getFreeHeight() >= (mLeftButtonsAbovePanel ? rowHeight : rowHeight + padding);
    if (abovePanel != mLeftButtonsAbovePanel && applyLeftButtonsArrangement(abovePanel))
      mLeftButtonsAbovePanel = abovePanel;
    updateSearchOptionsWidth(abovePanel, padding, metrics.getEndSlotWidth());

    final int marginStart = abovePanel ? 0 : getResources().getDimensionPixelSize(R.dimen.nav_menu_landscape_width);
    // mFrame is already padded by the navigation bar inset, so only the sheet's peek height is left.
    final int marginBottom = abovePanel ? Math.max(0, metrics.getPanelHeight() - mFrame.getPaddingBottom()) : 0;
    final int topTo = abovePanel ? ConstraintLayout.LayoutParams.UNSET : ConstraintLayout.LayoutParams.PARENT_ID;
    final int bottomTo = abovePanel ? ConstraintLayout.LayoutParams.PARENT_ID : ConstraintLayout.LayoutParams.UNSET;

    final ConstraintLayout.LayoutParams params =
        (ConstraintLayout.LayoutParams) mInnerLeftButtonsFrame.getLayoutParams();
    if (params.topToTop == topTo && params.bottomToBottom == bottomTo && params.getMarginStart() == marginStart
        && params.bottomMargin == marginBottom)
      return;
    params.topToTop = topTo;
    params.bottomToBottom = bottomTo;
    params.setMarginStart(marginStart);
    params.bottomMargin = marginBottom;
    mInnerLeftButtonsFrame.setLayoutParams(params);
  }

  // Beside the maneuver card the options strip has only the leftover screen width, which on narrow
  // landscape screens is less than the strip needs, so cap it there and let it scroll instead of
  // running off the edge. Inside the column it always fits at its full width.
  private void updateSearchOptionsWidth(boolean abovePanel, int padding, int endSlotWidth)
  {
    final View searchOptions = mFrame.findViewById(R.id.search_frame);
    if (searchOptions == null || mFrame.getWidth() == 0)
      return;
    // The strip shares the top row with the speed limit sign and, below it, the track recording
    // FAB, so keep clear of whichever of them holds that corner.
    final int endSlot = Math.max(endSlotWidth, trackRecordingSlotWidth(padding));
    final int fullWidth = getResources().getDimensionPixelSize(R.dimen.nav_search_options_width);
    final int leftover = mFrame.getWidth() - mFrame.getPaddingStart() - mFrame.getPaddingEnd()
                       - getResources().getDimensionPixelSize(R.dimen.nav_menu_landscape_width) - 2 * padding - endSlot;
    // Never shrink below the lead-in that clears the search button plus one category, otherwise
    // the strip opens empty and the button looks dead.
    final int minWidth = getResources().getDimensionPixelSize(R.dimen.nav_search_options_min_width);
    final int width = abovePanel ? fullWidth : Math.min(fullWidth, Math.max(minWidth, leftover));
    final ViewGroup.LayoutParams params = searchOptions.getLayoutParams();
    if (params.width == width)
      return;
    params.width = width;
    searchOptions.setLayoutParams(params);
  }

  private int trackRecordingSlotWidth(int padding)
  {
    if (mTrackRecordingStatusButton == null || !UiUtils.isVisible(mTrackRecordingStatusButton))
      return 0;
    return mTrackRecordingStatusButton.getWidth() + 2 * padding;
  }

  // Above the panel the buttons form a row reading bookmarks then search; beside the maneuver card
  // they stack vertically, search on top. The search options strip follows the search button either
  // way, since it is anchored to it.
  private boolean applyLeftButtonsArrangement(boolean row)
  {
    final View searchButton = mButtonsMap.get(MapButtons.search);
    final View bookmarksButton = mButtonsMap.get(MapButtons.bookmarks);
    if (searchButton == null || bookmarksButton == null)
      return false;
    final int gap = getResources().getDimensionPixelSize(R.dimen.margin_half);

    final ConstraintLayout.LayoutParams searchParams = (ConstraintLayout.LayoutParams) searchButton.getLayoutParams();
    searchParams.startToStart = row ? ConstraintLayout.LayoutParams.UNSET : ConstraintLayout.LayoutParams.PARENT_ID;
    searchParams.startToEnd = row ? R.id.btn_bookmarks : ConstraintLayout.LayoutParams.UNSET;
    searchParams.setMarginStart(row ? gap : 0);
    searchButton.setLayoutParams(searchParams);

    final ConstraintLayout.LayoutParams bookmarksParams =
        (ConstraintLayout.LayoutParams) bookmarksButton.getLayoutParams();
    bookmarksParams.topToTop = row ? ConstraintLayout.LayoutParams.PARENT_ID : ConstraintLayout.LayoutParams.UNSET;
    bookmarksParams.topToBottom = row ? ConstraintLayout.LayoutParams.UNSET : R.id.btn_search;
    bookmarksParams.topMargin = row ? 0 : gap;
    bookmarksButton.setLayoutParams(bookmarksParams);
    return true;
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
    // The navigation side column owns its own placement and is bottom-anchored there, so letting
    // applyMove pin its bottom to the sheet would throw it up over the maneuver card.
    if (mInnerLeftButtonsFrame != null && !mIsNavSideColumn
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
    mMapButtonsViewModel.getNavColumnMetrics().observe(viewLifecycleOwner, mNavColumnMetricsObserver);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation)
      mSearchOptionsButton.onResume();
    updateMenuBadge();
    updateLayerButton();
    updateHelpButtonIcon();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    if (mBlinkingAnimator != null)
    {
      mBlinkingAnimator.cancel();
      mBlinkingAnimator = null;
    }
  }

  public void onSearchOptionChange(@Nullable SearchOptionsButton.SearchOption searchOption)
  {
    if (searchOption == null && mMapButtonsViewModel.getLayoutMode().getValue() == LayoutMode.navigation)
      mSearchOptionsButton.reset();
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
    trackRecordingStatus
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

package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.OptIn;
import androidx.fragment.app.Fragment;

import com.google.android.material.badge.BadgeDrawable;
import com.google.android.material.badge.BadgeUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.UpdateInfo;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.maps.widget.placepage.PlacePageController;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;

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
  private MapLayersController mToggleMapLayerController;

  @Nullable
  private MyPositionButton mNavMyPosition;
  private SearchWheel mSearchWheel;
  private BadgeDrawable mBadgeDrawable;
  private float mContentHeight;
  private float mContentWidth;

  private MapButtonClickListener mMapButtonClickListener;
  private View.OnClickListener mOnSearchCanceledListener;
  private PlacePageController mPlacePageController;
  private OnBottomButtonsHeightChangedListener mOnBottomButtonsHeightChangedListener;

  private LayoutMode mLayoutMode;
  private int mMyPositionMode;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    if (mLayoutMode == LayoutMode.navigation)
      mFrame = inflater.inflate(R.layout.map_buttons_layout_navigation, container, false);
    else if (mLayoutMode == LayoutMode.planning)
      mFrame = inflater.inflate(R.layout.map_buttons_layout_planning, container, false);
    else
      mFrame = inflater.inflate(R.layout.map_buttons_layout_regular, container, false);

    mInnerLeftButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_left);
    mInnerRightButtonsFrame = mFrame.findViewById(R.id.map_buttons_inner_right);
    mBottomButtonsFrame = mFrame.findViewById(R.id.map_buttons_bottom);
    final View zoomFrame = mFrame.findViewById(R.id.zoom_buttons_container);
    mFrame.findViewById(R.id.nav_zoom_in)
          .setOnClickListener((v) -> mMapButtonClickListener.onClick(MapButtons.zoomIn));
    mFrame.findViewById(R.id.nav_zoom_out)
          .setOnClickListener((v) -> mMapButtonClickListener.onClick(MapButtons.zoomOut));
    final View bookmarksButton = mFrame.findViewById(R.id.btn_bookmarks);
    bookmarksButton.setOnClickListener((v) -> mMapButtonClickListener.onClick(MapButtons.bookmarks));
    final View myPosition = mFrame.findViewById(R.id.my_position);
    mNavMyPosition = new MyPositionButton(myPosition, mMyPositionMode, (v) -> mMapButtonClickListener.onClick(MapButtons.myPosition));

    // Some buttons do not exist in navigation mode
    final FloatingActionButton layersButton = mFrame.findViewById(R.id.layers_button);
    if (layersButton != null)
    {
      mToggleMapLayerController = new MapLayersController(layersButton,
                                                          () -> mMapButtonClickListener.onClick(MapButtons.toggleMapLayer), requireActivity());
    }
    final View menuButton = mFrame.findViewById(R.id.menu_button);
    if (menuButton != null)
    {
      menuButton.setOnClickListener((v) -> mMapButtonClickListener.onClick(MapButtons.menu));
      // This hack is needed to show the badge on the initial startup. For some reason, updateMenuBadge does not work from onResume() there.
      menuButton.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
        @Override
        public void onGlobalLayout() {
          updateMenuBadge();
          menuButton.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        }
      });
    }
    final View helpButton = mFrame.findViewById(R.id.help_button);
    if (helpButton != null)
      helpButton.setOnClickListener((v) -> mMapButtonClickListener.onClick(MapButtons.help));

    mSearchWheel = new SearchWheel(mFrame, (v) -> mMapButtonClickListener.onClick(MapButtons.search), mOnSearchCanceledListener);
    final View searchButton = mFrame.findViewById(R.id.btn_search);

    // Used to get the maximum height the buttons will evolve in
    mFrame.addOnLayoutChangeListener(new MapButtonsController.ContentViewLayoutChangeListener(mFrame));

    mButtonsMap = new HashMap<>();
    mButtonsMap.put(MapButtons.zoom, zoomFrame);
    mButtonsMap.put(MapButtons.myPosition, myPosition);
    mButtonsMap.put(MapButtons.bookmarks, bookmarksButton);
    mButtonsMap.put(MapButtons.search, searchButton);

    if (layersButton != null)
      mButtonsMap.put(MapButtons.toggleMapLayer, layersButton);
    if (menuButton != null)
      mButtonsMap.put(MapButtons.menu, menuButton);
    if (helpButton != null)
      mButtonsMap.put(MapButtons.help, helpButton);
    return mFrame;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    showMapButtons(true);
  }

  public LayoutMode getLayoutMode()
  {
    return mLayoutMode;
  }

  public void init(LayoutMode layoutMode, int myPositionMode, MapButtonClickListener mapButtonClickListener, @NonNull View.OnClickListener onSearchCanceledListener, PlacePageController placePageController, OnBottomButtonsHeightChangedListener onBottomButtonsHeightChangedListener)
  {
    mLayoutMode = layoutMode;
    mMyPositionMode = myPositionMode;
    mMapButtonClickListener = mapButtonClickListener;
    mOnSearchCanceledListener = onSearchCanceledListener;
    mPlacePageController = placePageController;
    mOnBottomButtonsHeightChangedListener = onBottomButtonsHeightChangedListener;
  }

  public void showButton(boolean show, MapButtonsController.MapButtons button)
  {
    // TODO(AB): Why do we need this check? Isn't it better to crash and fix the wrong logic ASAP?
    final View buttonView = mButtonsMap.get(button);
    if (buttonView == null)
      return;
    switch (button)
    {
      case zoom:
        UiUtils.showIf(show && Config.showZoomButtons(), buttonView);
        break;
      case toggleMapLayer:
        if (mToggleMapLayerController != null)
          mToggleMapLayerController.showButton(show && !isInNavigationMode());
        break;
      case myPosition:
        if (mNavMyPosition != null)
          mNavMyPosition.showButton(show);
        break;
      case search:
        mSearchWheel.show(show);
      case bookmarks:
      case menu:
        UiUtils.showIf(show, buttonView);
    }
  }

  private static int dpToPx(float dp, Context context)
  {
    return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, context.getResources().getDisplayMetrics());
  }

  @OptIn(markerClass = com.google.android.material.badge.ExperimentalBadgeUtils.class)
  public void updateMenuBadge()
  {
    final View menuButton = mButtonsMap.get(MapButtons.menu);
    final Context context = requireContext();
    if (menuButton == null || context == null)
      return;
    final UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    final int count = (info == null ? 0 : info.filesCount);
    final int verticalOffset = dpToPx(8, context) + dpToPx(Integer.toString(count).length() * 5, context);
    BadgeUtils.detachBadgeDrawable(mBadgeDrawable, menuButton);
    mBadgeDrawable = BadgeDrawable.create(context);
    mBadgeDrawable.setMaxCharacterCount(3);
    mBadgeDrawable.setHorizontalOffset(verticalOffset);
    mBadgeDrawable.setVerticalOffset(dpToPx(9, context));
    mBadgeDrawable.setNumber(count);
    mBadgeDrawable.setVisible(count > 0);
    BadgeUtils.attachBadgeDrawable(mBadgeDrawable, menuButton);
  }

  private boolean isBehindPlacePage(View v)
  {
    return !(mContentWidth / 2 > (mPlacePageController.getPlacePageWidth() / 2.0) + v.getWidth());
  }

  private boolean isMoving(View v)
  {
    return v.getTranslationY() < 0;
  }

  public void move(float translationY)
  {
    if (mContentHeight == 0)
      return;

    // Move the buttons containers to follow the place page
    if (mInnerRightButtonsFrame != null &&
        (isBehindPlacePage(mInnerRightButtonsFrame) || isMoving(mInnerRightButtonsFrame)))
      applyMove(mInnerRightButtonsFrame, translationY);
    if (mInnerLeftButtonsFrame != null &&
        (isBehindPlacePage(mInnerLeftButtonsFrame) || isMoving(mInnerLeftButtonsFrame)))
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
    updateButtonsVisibility(mInnerLeftButtonsFrame.getTranslationY(), mInnerLeftButtonsFrame);
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
        showButton(getViewTopOffset(translation, button) > 0, entry.getKey());
    }
  }

  public float getBottomButtonsHeight()
  {
    if (mBottomButtonsFrame != null && mFrame != null && UiUtils.isVisible(mFrame))
      return mBottomButtonsFrame.getMeasuredHeight();
    else
      return 0;
  }

  public void showMapButtons(boolean show)
  {
    if (show)
    {
      UiUtils.show(mFrame);
      showButton(true, MapButtons.zoom);
    }
    else
      UiUtils.hide(mFrame);
    mOnBottomButtonsHeightChangedListener.OnBottomButtonsHeightChanged();
  }

  private boolean isInNavigationMode()
  {
    return RoutingController.get().isPlanning() || RoutingController.get().isNavigating();
  }

  public void toggleMapLayer(@NonNull Mode mode)
  {
    if (mToggleMapLayerController != null)
      mToggleMapLayerController.toggleMode(mode);
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

  public void onResume()
  {
    super.onResume();
    mSearchWheel.onResume();
    updateMenuBadge();
  }

  public void resetSearch()
  {
    mSearchWheel.reset();
  }

  public void saveNavSearchState(@NonNull Bundle outState)
  {
    mSearchWheel.saveState(outState);
  }

  public void restoreNavSearchState(@NonNull Bundle savedInstanceState)
  {
    mSearchWheel.restoreState(savedInstanceState);
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
    help
  }

  public interface MapButtonClickListener
  {
    void onClick(MapButtons button);
  }

  public interface OnBottomButtonsHeightChangedListener
  {
    void OnBottomButtonsHeightChanged();
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
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                               int oldTop, int oldRight, int oldBottom)
    {
      mContentHeight = bottom - top;
      mContentWidth = right - left;
      mOnBottomButtonsHeightChangedListener.OnBottomButtonsHeightChanged();
      mContentView.removeOnLayoutChangeListener(this);
    }
  }
}

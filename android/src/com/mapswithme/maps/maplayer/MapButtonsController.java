package com.mapswithme.maps.maplayer;

import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.SearchWheel;
import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.maps.widget.placepage.PlacePageController;
import com.mapswithme.util.Config;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class MapButtonsController
{
  @NonNull
  private final View mButtonsFrame;
  @NonNull
  private final View mZoomFrame;
  @NonNull
  private final ImageButton mLayersButton;
  @NonNull
  private final View myPosition;
  @NonNull
  private final View mSearchButtonFrame;
  @Nullable
  private final MyPositionButton mNavMyPosition;
  @NonNull
  private final MapLayersController mToggleMapLayerController;
  @NonNull
  private final SearchWheel mSearchWheel;
  private final PlacePageController mPlacePageController;
  private final float mBottomMargin;
  private final int mButtonWidth;
  private final float mInitialButtonMargin;
  private float mTopLimit;
  private float mContentHeight;
  private float mContentWidth;

  public MapButtonsController(@NonNull View frame, AppCompatActivity activity, MapButtonClickListener mapButtonClickListener, PlacePageController placePageController)
  {
    mButtonsFrame = frame.findViewById(R.id.navigation_buttons_inner);
    mZoomFrame = frame.findViewById(R.id.zoom_buttons_container);
    mPlacePageController = placePageController;
    frame.findViewById(R.id.nav_zoom_in)
         .setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.zoomIn));
    frame.findViewById(R.id.nav_zoom_out)
         .setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.zoomOut));
    myPosition = frame.findViewById(R.id.my_position);
    mNavMyPosition = new MyPositionButton(myPosition, (v) -> mapButtonClickListener.onClick(MapButtons.myPosition));

    mLayersButton = frame.findViewById(R.id.layers_button);
    mToggleMapLayerController = new MapLayersController(mLayersButton,
                                                        () -> mapButtonClickListener.onClick(MapButtons.toggleMapLayer), activity);

    mSearchButtonFrame = activity.findViewById(R.id.search_button_frame);
    mSearchWheel = new SearchWheel(frame, (v) -> mapButtonClickListener.onClick(MapButtons.navSearch));
    ImageView bookmarkButton = mSearchButtonFrame.findViewById(R.id.btn_bookmarks);
    bookmarkButton.setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.navBookmarks));
    bookmarkButton.setImageDrawable(Graphics.tint(bookmarkButton.getContext(), R.drawable.ic_menu_bookmarks));

    // Used to get the maximum height the buttons will evolve in
    frame.addOnLayoutChangeListener(new MapButtonsController.ContentViewLayoutChangeListener(frame));
    mBottomMargin = ((RelativeLayout.LayoutParams) mButtonsFrame.getLayoutParams()).bottomMargin;
    mInitialButtonMargin = ((ConstraintLayout.LayoutParams) mZoomFrame.getLayoutParams()).bottomMargin;
    mButtonWidth = mLayersButton.getLayoutParams().width;
  }

  public void showButton(boolean show, MapButtonsController.MapButtons button)
  {
    switch (button)
    {
      case zoom:
        UiUtils.showIf(show && Config.showZoomButtons(), mZoomFrame);
        break;
      case toggleMapLayer:
        mToggleMapLayerController.showButton(show && !isInNavigationMode());
        break;
      case myPosition:
        if (mNavMyPosition != null)
          mNavMyPosition.showButton(show);
        break;
      case nav:
        UiUtils.showIf(show && isInNavigationMode(),
                       mSearchButtonFrame);
    }
  }

  private boolean isScreenWideEnough()
  {
    return mContentWidth > (mPlacePageController.getPlacePageWidth() + 2 * mButtonWidth);
  }

  public void move(float translationY)
  {
    if (mContentHeight == 0 || isScreenWideEnough())
      return;

    // Move the buttons container to follow the place page
    final float translation = mBottomMargin + translationY - mContentHeight;
    final float appliedTranslation = translation <= 0 ? translation : 0;
    mButtonsFrame.setTranslationY(appliedTranslation);

    // Reduce buttons margin to move them only if necessary
    // Zoom frame is above the layers so if must move twice as much
    final float appliedMarginTranslationLayers = Math.min(-appliedTranslation, mInitialButtonMargin);
    final float maxZoomTranslation = UiUtils.isVisible(mLayersButton) ? 2 * mInitialButtonMargin : mInitialButtonMargin;
    final float appliedMarginTranslationZoomFrame = Math.min(-appliedTranslation, maxZoomTranslation);
    mLayersButton.setTranslationY(appliedMarginTranslationLayers);
    mSearchButtonFrame.setTranslationY(appliedMarginTranslationLayers);
    mZoomFrame.setTranslationY(appliedMarginTranslationZoomFrame);

    updateButtonsVisibility(appliedTranslation);
  }

  public void updateButtonsVisibility()
  {
    updateButtonsVisibility(mButtonsFrame.getTranslationY());
  }

  private void updateButtonsVisibility(final float translation)
  {
    showButton(getViewTopOffset(translation, mZoomFrame) > 0, MapButtons.zoom);
    showButton(getViewTopOffset(translation, mSearchButtonFrame) > 0, MapButtons.nav);
    showButton(getViewTopOffset(translation, mLayersButton) > 0, MapButtons.toggleMapLayer);
    showButton(getViewTopOffset(translation, myPosition) > 0, MapButtons.myPosition);
  }

  public void setTopLimit(float limit)
  {
    mTopLimit = limit;
    updateButtonsVisibility();
  }

  public void showMapButtons(boolean show)
  {
    if (show)
    {
      UiUtils.show(mButtonsFrame);
      showButton(true, MapButtons.zoom);
    }
    else
      UiUtils.hide(mButtonsFrame);
  }

  private boolean isInNavigationMode()
  {
    return RoutingController.get().isPlanning() || RoutingController.get().isNavigating();
  }

  public void toggleMapLayer(@NonNull Mode mode)
  {
    mToggleMapLayerController.toggleMode(mode);
  }

  public void updateNavMyPositionButton(int newMode)
  {
    if (mNavMyPosition != null)
      mNavMyPosition.update(newMode);
  }

  private int getViewTopOffset(float translation, View v)
  {
    return (int) (translation + v.getTop() - mTopLimit);
  }

  public void onResume()
  {
    showButton(true, MapButtons.zoom);
    mSearchWheel.onResume();
  }

  public void resetNavSearch()
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

  public enum MapButtons
  {
    myPosition,
    toggleMapLayer,
    zoomIn,
    zoomOut,
    zoom,
    nav,
    navSearch,
    navBookmarks
  }

  public interface MapButtonClickListener
  {
    void onClick(MapButtons button);
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
      mContentView.removeOnLayoutChangeListener(this);
    }
  }
}

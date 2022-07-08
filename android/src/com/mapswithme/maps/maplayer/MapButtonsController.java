package com.mapswithme.maps.maplayer;

import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.mapswithme.maps.NavigationButtonsAnimationController;
import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.SearchWheel;
import com.mapswithme.maps.widget.menu.MyPositionButton;
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
  @Nullable
  private final NavigationButtonsAnimationController mNavAnimationController;
  @NonNull
  private final SearchWheel mSearchWheel;

  private float mTopLimit;

  public MapButtonsController(@NonNull View frame, AppCompatActivity activity, MapButtonClickListener mapButtonClickListener)
  {
    mButtonsFrame = frame.findViewById(R.id.navigation_buttons_inner);
    mZoomFrame = frame.findViewById(R.id.zoom_buttons_container);
    View zoomInButton = frame.findViewById(R.id.nav_zoom_in);
    zoomInButton.setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.zoomIn));
    View zoomOutButton = frame.findViewById(R.id.nav_zoom_out);
    zoomOutButton.setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.zoomOut));
    myPosition = frame.findViewById(R.id.my_position);
    mNavMyPosition = new MyPositionButton(myPosition, (v) -> mapButtonClickListener.onClick(MapButtons.myPosition));

    mLayersButton = frame.findViewById(R.id.layers_button);
    mToggleMapLayerController = new MapLayersController(mLayersButton,
                                                        () -> mapButtonClickListener.onClick(MapButtons.toggleMapLayer), activity);

    mNavAnimationController = new NavigationButtonsAnimationController(
        mButtonsFrame, this::onTranslationChanged);

    mSearchButtonFrame = activity.findViewById(R.id.search_button_frame);
    mSearchWheel = new SearchWheel(frame, (v) -> mapButtonClickListener.onClick(MapButtons.navSearch));
    ImageView bookmarkButton = mSearchButtonFrame.findViewById(R.id.btn_bookmarks);
    bookmarkButton.setOnClickListener((v) -> mapButtonClickListener.onClick(MapButtons.navBookmarks));
    bookmarkButton.setImageDrawable(Graphics.tint(bookmarkButton.getContext(),
                                                  R.drawable.ic_menu_bookmarks));
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

  public void move(float translationY)
  {
    if (mNavAnimationController != null)
      mNavAnimationController.move(translationY);
  }

  public void setTopLimit(float limit)
  {
    mTopLimit = limit;
    if (mNavAnimationController != null)
      mNavAnimationController.update();
  }

  public void showMapButtons(boolean show)
  {
    if (show)
    {
      UiUtils.show(mButtonsFrame);
      showButton(true, MapButtons.zoom);
    }
    else
    {
      UiUtils.hide(mButtonsFrame);
    }
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

  private void onTranslationChanged(float translation)
  {
    showButton(getViewTopOffset(translation, mZoomFrame) > 0, MapButtons.zoom);
    showButton(getViewTopOffset(translation, mSearchButtonFrame) > 0, MapButtons.nav);
    showButton(getViewTopOffset(translation, mLayersButton) > 0, MapButtons.toggleMapLayer);
    showButton(getViewTopOffset(translation, myPosition) > 0, MapButtons.myPosition);
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
}

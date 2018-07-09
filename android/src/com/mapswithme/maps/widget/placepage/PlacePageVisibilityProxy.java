package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;


public class PlacePageVisibilityProxy implements BasePlacePageAnimationController.OnVisibilityChangedListener
{
  @NonNull
  private final BasePlacePageAnimationController.OnVisibilityChangedListener mListener;
  @Nullable
  private final BannerController mBannerController;

  public PlacePageVisibilityProxy(@NonNull BasePlacePageAnimationController.OnVisibilityChangedListener listener,
                                  @Nullable BannerController bannerController)
  {
    mListener = listener;
    mBannerController = bannerController;
  }

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    if (mBannerController != null)
      mBannerController.onChangedVisibility(isVisible);
    mListener.onPreviewVisibilityChanged(isVisible);

  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    mListener.onPlacePageVisibilityChanged(isVisible);
  }
}

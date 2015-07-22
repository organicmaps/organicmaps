package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;

import com.mapswithme.util.UiUtils;
import com.nineoldandroids.view.ViewHelper;

public class LeftCompatAnimationController extends LeftFullPlacePageAnimationController
{
  public LeftCompatAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
  }

  @Override
  protected void hidePlacePage()
  {
    ViewHelper.setTranslationX(mPlacePage, -mPlacePage.getWidth());
    mPlacePage.clearAnimation(); // need clear animation to correctly hide view on pre-honeycomb devices.
    UiUtils.hide(mPlacePage);
    mIsPlacePageVisible = mIsPreviewVisible = false;
    notifyVisibilityListener();
  }
}

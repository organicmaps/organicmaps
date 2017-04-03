package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class BaseMwmNativeAd implements MwmNativeAd
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BaseMwmNativeAd.class.getSimpleName();

  @Override
  public void registerView(@NonNull View bannerView)
  {
    View largeAction = bannerView.findViewById(R.id.tv__action_large);
    if (UiUtils.isVisible(largeAction))
    {
      LOGGER.d(TAG, "Register the large action button for '" + getBannerId() + "'");
      registerViewForInteraction(largeAction);
      return;
    }

    View actionSmall = bannerView.findViewById(R.id.tv__action_small);
    if (UiUtils.isVisible(actionSmall))
    {
      LOGGER.d(TAG, "Register the small action button for '" + getBannerId() + "'");
      registerViewForInteraction(actionSmall);
    }
  }

  abstract void registerViewForInteraction(@NonNull View view);
}

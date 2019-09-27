package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
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
  public void registerView(@NonNull View view)
  {
    View largeAction = view.findViewById(R.id.tv__action_large);
    if (UiUtils.isVisible(largeAction))
    {
      LOGGER.d(TAG, "Register the large action button for '" + getBannerId() + "'");
      register(largeAction);
      return;
    }

    View smallAction = view.findViewById(R.id.tv__action_small);
    if (UiUtils.isVisible(smallAction))
    {
      LOGGER.d(TAG, "Register the small action button for '" + getBannerId() + "'");
      register(smallAction);
    }
  }

  @Override
  public void unregisterView(@NonNull View bannerView)
  {
    View largeAction = bannerView.findViewById(R.id.tv__action_large);
    if (UiUtils.isVisible(largeAction))
    {
      LOGGER.d(TAG, "Unregister the large action button for '" + getBannerId() + "'");
      unregister(largeAction);
      return;
    }

    View smallAction = bannerView.findViewById(R.id.tv__action_small);
    if (UiUtils.isVisible(smallAction))
    {
      LOGGER.d(TAG, "Unregister the small action button for '" + getBannerId() + "'");
      unregister(smallAction);
    }
  }

  abstract void register(@NonNull View view);

  abstract void unregister(@NonNull View view);

  @Override
  public String toString()
  {
    return "Ad title: " + getTitle();
  }
}

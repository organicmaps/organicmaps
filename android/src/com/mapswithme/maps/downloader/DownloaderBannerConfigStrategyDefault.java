package com.mapswithme.maps.downloader;

import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;

class DownloaderBannerConfigStrategyDefault implements DownloaderBannerConfigStrategy
{
  @Override
  public void configureView(@NonNull View parent, @IdRes int iconViewId, @IdRes int messageViewId,
                            @IdRes int buttonViewId)
  {
  }
}

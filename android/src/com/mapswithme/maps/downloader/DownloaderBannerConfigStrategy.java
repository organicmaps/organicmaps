package com.mapswithme.maps.downloader;

import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;

interface DownloaderBannerConfigStrategy
{
  void configureView(@NonNull View parent, @IdRes int iconViewId, @IdRes int messageViewId,
                     @IdRes int buttonViewId);
}

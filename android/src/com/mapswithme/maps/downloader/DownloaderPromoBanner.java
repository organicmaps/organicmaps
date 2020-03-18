package com.mapswithme.maps.downloader;

import android.graphics.Color;

import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.StatisticValueConverter;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Promo banner for on-map downloader. Created by native code.
 */
public final class DownloaderPromoBanner
{
  @NonNull
  private final DownloaderBannerType mType;
  @NonNull
  private final String mUrl;

  // Note: must be corresponded to ads::Banner::Type in ads/banner.hpp
  private static DownloaderBannerType fromCoreType(int coreValue)
  {
    switch (coreValue)
    {
      case 4: return DownloaderBannerType.TINKOFF_AIRLINES;
      case 5: return DownloaderBannerType.TINKOFF_INSURANCE;
      case 6: return DownloaderBannerType.MTS;
      case 7: return DownloaderBannerType.SKYENG;
      case 8: return DownloaderBannerType.BOOKMARK_CATALOG;
      default: throw new AssertionError("Incorrect core banner type: " + coreValue);
    }
  }

  // Note: this constructor must be called from jni only.
  private DownloaderPromoBanner(int coreType, @NonNull String url)
  {
    this.mType = fromCoreType(coreType);
    this.mUrl = url;
  }

  @NonNull
  public DownloaderBannerType getType() { return mType; }

  @NonNull
  public String getUrl() { return mUrl; }
}

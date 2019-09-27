package com.mapswithme.maps.downloader;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import com.mapswithme.util.statistics.StatisticValueConverter;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Promo banner for on-map downloader. Created by native code.
 */
public final class DownloaderPromoBanner implements StatisticValueConverter<String>
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ DOWNLOADER_PROMO_TYPE_NO_PROMO, DOWNLOADER_PROMO_TYPE_BOOKMARK_CATALOG,
            DOWNLOADER_PROMO_TYPE_MEGAFON })
  public @interface DownloaderPromoType {}

  // Must be corresponded to DownloaderPromoType in downloader_promo.hpp
  public static final int DOWNLOADER_PROMO_TYPE_NO_PROMO = 0;
  public static final int DOWNLOADER_PROMO_TYPE_BOOKMARK_CATALOG = 1;
  public static final int DOWNLOADER_PROMO_TYPE_MEGAFON = 2;

  @DownloaderPromoType
  private final int mType;

  @NonNull
  private final String mUrl;

  public DownloaderPromoBanner(@DownloaderPromoType int type, @NonNull String url)
  {
    this.mType = type;
    this.mUrl = url;
  }

  @DownloaderPromoType
  public int getType() { return mType; }

  @NonNull
  public String getUrl() { return mUrl; }

  @NonNull
  @Override
  public String toStatisticValue()
  {
    return DownloaderPromoBannerStats.values()[getType()].getValue();
  }
}

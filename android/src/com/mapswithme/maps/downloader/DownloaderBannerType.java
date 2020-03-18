package com.mapswithme.maps.downloader;

import android.graphics.Color;

import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.StatisticValueConverter;
import com.mapswithme.util.statistics.Statistics;

enum DownloaderBannerType implements StatisticValueConverter<String>
{
  TINKOFF_AIRLINES(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_tinkoff,
                                                             R.string.tinkoff_allairlines_map_downloader_title,
                                                             R.string.tinkoff_allairlines_map_downloader_cta_button,
                                                             Color.parseColor("#000000"),
                                                             Color.parseColor("#FFDD2D")))
  {
    @NonNull
    @Override
    public String toStatisticValue()
    {
      return Statistics.ParamValue.TINKOFF_ALL_AIRLINES;
    }
  },
  TINKOFF_INSURANCE(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_tinkoff,
                                                              R.string.tinkoff_insurance_map_downloader_title,
                                                              R.string.tinkoff_insurance_map_downloader_cta_button,
                                                              Color.parseColor("#000000"),
                                                              Color.parseColor("#FFDD2D")))
  {
    @NonNull
    @Override
    public String toStatisticValue()
    {
      return Statistics.ParamValue.TINKOFF_INSURANCE;
    }
  },
  MTS(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_mts,
                                                R.string.mts_map_downloader_title,
                                                R.string.mts_map_downloader_cta_button,
                                                Color.parseColor("#FFFFFF"),
                                                Color.parseColor("#E30611")))
  {
    @NonNull
    @Override
    public String toStatisticValue()
    {
      return Statistics.ParamValue.SKYENG;
    }
  },
  SKYENG(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_skyeng,
                                                   R.string.skyeng_map_downloader_title,
                                                   R.string.skyeng_map_downloader_cta_button,
                                                   Color.parseColor("#FFFFFF"),
                                                   Color.parseColor("#4287DF")))
  {
    @NonNull
    @Override
    public String toStatisticValue()
    {
      return Statistics.ParamValue.MTS;
    }
  },
  BOOKMARK_CATALOG(new DownloaderBannerConfigStrategyDefault())
  {
    @NonNull
    @Override
    public String toStatisticValue()
    {
      return Statistics.ParamValue.MAPSME_GUIDES;
    }
  };

  @NonNull
  private final DownloaderBannerConfigStrategy mViewConfigStrategy;

  DownloaderBannerType(@NonNull DownloaderBannerConfigStrategy viewConfigStrategy)
  {
    mViewConfigStrategy = viewConfigStrategy;
  }

  @NonNull
  DownloaderBannerConfigStrategy getViewConfigStrategy() { return mViewConfigStrategy; }
}

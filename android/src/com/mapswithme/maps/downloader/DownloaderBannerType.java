package com.mapswithme.maps.downloader;

import android.app.Activity;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.StatisticValueConverter;
import com.mapswithme.util.statistics.Statistics;

enum DownloaderBannerType implements StatisticValueConverter<String>
{
  TINKOFF_AIRLINES(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_tinkoff,
                                                             R.string.tinkoff_allairlines_map_downloader_title,
                                                             R.string.tinkoff_allairlines_map_downloader_cta_button,
                                                             R.color.black_primary,
                                                             R.color.tinkoff_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.TINKOFF_ALL_AIRLINES;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner;
        }
      },
  TINKOFF_INSURANCE(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_tinkoff,
                                                              R.string.tinkoff_insurance_map_downloader_title,
                                                              R.string.tinkoff_insurance_map_downloader_cta_button,
                                                              R.color.black_primary,
                                                              R.color.tinkoff_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.TINKOFF_INSURANCE;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner;
        }
      },
  MTS(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_mts,
                                                R.string.mts_map_downloader_title,
                                                R.string.mts_map_downloader_cta_button,
                                                R.color.white_primary,
                                                R.color.mts_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.MTS;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner;
        }
      },
  SKYENG(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_skyeng,
                                                   R.string.skyeng_map_downloader_title,
                                                   R.string.skyeng_map_downloader_cta_button,
                                                   R.color.white_primary,
                                                   R.color.skyeng_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.SKYENG;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner;
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

        @Override
        void onAction(@NonNull Activity context, @NonNull String url)
        {
          BookmarksCatalogActivity.startForResult(context,
                                                  BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                                  url);
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_catalog_banner;
        }
      },
  MASTERCARD_SBERBANK(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_mastercard_sberbank,
                                                                R.string.sberbank_map_downloader_title,
                                                                R.string.sberbank_map_downloader_cta_button,
                                                                R.color.white_primary,
                                                                R.color.mastercard_sberbank_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.MASTERCARD_SBERBANK;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner_big_logo;
        }
      },
  ARSENAL_MEDIC(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_arsenal,
                                                          R.string.arsenal_telemed_map_downloader_title,
                                                          R.string.arsenal_cta_button,
                                                          R.color.white_primary,
                                                          R.color.arsenal_button))
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return Statistics.ParamValue.ARSENAL_MEDIC;
        }

        @LayoutRes
        @Override
        int getLayoutId()
        {
          return R.layout.onmap_downloader_banner;
        }
      },
  ARSENAL_FLAT(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_arsenal,
                                                         R.string.arsenal_flat_map_downloader_title,
                                                         R.string.arsenal_cta_button,
                                                         R.color.white_primary,
                                                         R.color.arsenal_button))
    {
      @NonNull
      @Override
      public String toStatisticValue()
      {
        return Statistics.ParamValue.ARSENAL_FLAT;
      }

      @LayoutRes
      @Override
      int getLayoutId()
      {
        return R.layout.onmap_downloader_banner;
      }
    },
  ARSENAL_INSURANCE_CRIMEA(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_arsenal,
                                                                     R.string.arsenal_crimea_map_downloader_title,
                                                                     R.string.arsenal_cta_button,
                                                                     R.color.white_primary,
                                                                     R.color.arsenal_button))
    {
      @NonNull
      @Override
      public String toStatisticValue()
      {
        return Statistics.ParamValue.ARSENAL_INSURANCE_CRIMEA;
      }

      @LayoutRes
      @Override
      int getLayoutId()
      {
        return R.layout.onmap_downloader_banner;
      }
    },
  ARSENAL_INSURANCE_RUSSIA(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_arsenal,
                                                                     R.string.arsenal_russia_map_downloader_title,
                                                                     R.string.arsenal_cta_button,
                                                                     R.color.white_primary,
                                                                     R.color.arsenal_button))
    {
      @NonNull
      @Override
      public String toStatisticValue()
      {
        return Statistics.ParamValue.ARSENAL_INSURANCE_RUSSIA;
      }

      @LayoutRes
      @Override
      int getLayoutId()
      {
        return R.layout.onmap_downloader_banner;
      }
    },
  ARSENAL_INSURANCE_WORLD(new DownloaderBannerConfigStrategyPartner(R.drawable.ic_logo_arsenal,
                                                                    R.string.arsenal_abroad_map_downloader_title,
                                                                    R.string.arsenal_cta_button,
                                                                    R.color.white_primary,
                                                                    R.color.arsenal_button))
    {
      @NonNull
      @Override
      public String toStatisticValue()
      {
        return Statistics.ParamValue.ARSENAL_INSURANCE_WORLD;
      }

      @LayoutRes
      @Override
      int getLayoutId()
      {
        return R.layout.onmap_downloader_banner;
      }
    };

  @NonNull
  private final DownloaderBannerConfigStrategy mViewConfigStrategy;

  DownloaderBannerType(@NonNull DownloaderBannerConfigStrategy viewConfigStrategy)
  {
    mViewConfigStrategy = viewConfigStrategy;
  }

  @NonNull
  DownloaderBannerConfigStrategy getViewConfigStrategy()
  {
    return mViewConfigStrategy;
  }

  void onAction(@NonNull Activity context, @NonNull String url)
  {
    Utils.openUrl(context, url);
  }

  @LayoutRes
  abstract int getLayoutId();
}

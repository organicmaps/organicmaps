package com.mapswithme.maps.taxi;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

public enum TaxiType
{
  UBER
      {
        @NonNull
        public String getPackageName()
        {
          return "com.ubercab";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Direct;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_logo_uber;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.uber;
        }

        @NonNull
        public String getProviderName()
        {
          return "Uber";
        }
      },
  YANDEX(new LocaleDependentFormatPriceStrategy(), true)
      {
        @NonNull
        public String getPackageName()
        {
          return "ru.yandex.taxi";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Indirect;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_logo_yandex_taxi;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.yandex_taxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Yandex";
        }
      },
  MAXIM(true)
      {
        @NonNull
        public String getPackageName()
        {
          return "com.taxsee.taxsee";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Direct;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_taxi_logo_maksim;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.maxim_taxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Maxim";
        }
      },
  TAXI_VEZET(R.string.place_page_starting_from, new LocaleDependentFormatPriceStrategy(), true)
      {
        @NonNull
        public String getPackageName()
        {
          return "ru.rutaxi.vezet";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Direct;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_taxi_logo_vezet;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.vezet_taxi;
        }

        @NonNull
        public String getProviderName()
        {
          return "Vezet";
        }
      },
  FREENOW
      {
        @NonNull
        public String getPackageName()
        {
          return "taxi.android.client";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Indirect;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_logo_freenow;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.freenow_taxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Freenow";
        }
      },
  YANGO(new LocaleDependentFormatPriceStrategy(), true)
      {
        @NonNull
        public String getPackageName()
        {
          return "com.yandex.yango";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Indirect;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_logo_yango;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.yango_taxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Yango";
        }
      },
  CITYMOBIL(new LocaleDependentFormatPriceStrategy(), false)
      {
        @NonNull
        public String getPackageName()
        {
          return "com.citymobil";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Indirect;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_logo_citymobil;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.citymobil_taxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Citymobil";
        }
      };

  @StringRes
  private final int mWaitingTemplateResId;
  @NonNull
  private final FormatPriceStrategy mFormatPriceStrategy;
  private final boolean mPriceApproximated;

  TaxiType(@StringRes int waitingTemplateResId, @NonNull FormatPriceStrategy strategy,
           boolean priceApproximated)
  {
    mWaitingTemplateResId = waitingTemplateResId;
    mFormatPriceStrategy = strategy;
    mPriceApproximated = priceApproximated;
  }

  TaxiType(@NonNull FormatPriceStrategy strategy, boolean priceApproximated)
  {
    this(R.string.taxi_wait, strategy, priceApproximated);
  }

  TaxiType(boolean priceApproximated)
  {
    this(new DefaultFormatPriceStrategy(), priceApproximated);
  }

  TaxiType()
  {
    this(false);
  }

  @NonNull
  public abstract String getPackageName();

  @NonNull
  public abstract Utils.PartnerAppOpenMode getOpenMode();

  @DrawableRes
  public abstract int getIcon();

  @StringRes
  public abstract int getTitle();

  @NonNull
  public abstract String getProviderName();

  @StringRes
  public int getWaitingTemplateResId()
  {
    return mWaitingTemplateResId;
  }

  public boolean isPriceApproximated()
  {
    return mPriceApproximated;
  }

  @NonNull
  public FormatPriceStrategy getFormatPriceStrategy()
  {
    return mFormatPriceStrategy;
  }
}

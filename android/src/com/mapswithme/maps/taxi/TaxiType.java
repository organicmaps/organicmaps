package com.mapswithme.maps.taxi;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;

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
  YANDEX(new LocaleDependentFormatPriceStrategy())
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
  MAXIM
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
  RUTAXI(R.string.place_page_starting_from, new LocaleDependentFormatPriceStrategy())
      {
        @NonNull
        public String getPackageName()
        {
          return "com.its.rto";
        }

        @NonNull
        public Utils.PartnerAppOpenMode getOpenMode()
        {
          return Utils.PartnerAppOpenMode.Direct;
        }

        @DrawableRes
        public int getIcon()
        {
          return R.drawable.ic_taxi_logo_rutaxi;
        }

        @StringRes
        public int getTitle()
        {
          return R.string.rutaxi_title;
        }

        @NonNull
        public String getProviderName()
        {
          return "Rutaxi";
        }
      };

  private static final Collection<TaxiType> APPROXIMATE_PRICE_TAXI_TYPES =
      Collections.unmodifiableSet(new HashSet<>(Arrays.asList(YANDEX, MAXIM, RUTAXI)));

  @StringRes
  private final int mWaitingTemplateResId;
  @NonNull
  private final FormatPriceStrategy mFormatPriceStrategy;

  TaxiType(@StringRes int waitingTemplateResId, @NonNull FormatPriceStrategy strategy)
  {
    mWaitingTemplateResId = waitingTemplateResId;
    mFormatPriceStrategy = strategy;
  }

  TaxiType(@NonNull FormatPriceStrategy strategy)
  {
    this(R.string.taxi_wait, strategy);
  }

  TaxiType()
  {
    this(R.string.taxi_wait, new DefaultFormatPriceStrategy());
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

  public boolean isApproximatePrice()
  {
    return APPROXIMATE_PRICE_TAXI_TYPES.contains(this);
  }

  // For Uber and Maxim we don't do formatting, because Uber and Maxim does it on its side.
  @NonNull
  public FormatPriceStrategy getFormatPriceStrategy()
  {
    return mFormatPriceStrategy;
  }
}

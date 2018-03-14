package com.mapswithme.maps.taxi;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

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
  YANDEX
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
      };

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
}

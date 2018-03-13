package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

public enum TaxiType
{
  PROVIDER_UBER
  {
    @NonNull
    public String getPackageName() { return "com.ubercab"; }
    @NonNull
    public Utils.PartnerAppOpenMode getOpenMode() { return Utils.PartnerAppOpenMode.Direct; }
    public int getIcon() { return R.drawable.ic_logo_uber; }
    public int getTitle() { return R.string.uber; }
    @NonNull
    public String getProviderName() { return "Uber"; }
  },
  PROVIDER_YANDEX
  {
    @NonNull
    public String getPackageName() { return "ru.yandex.taxi"; }
    @NonNull
    public Utils.PartnerAppOpenMode getOpenMode() { return Utils.PartnerAppOpenMode.Indirect; }
    public int getIcon() { return R.drawable.ic_logo_yandex_taxi; }
    public int getTitle() { return R.string.yandex_taxi_title; }
    @NonNull
    public String getProviderName() { return "Yandex"; }
  },
  PROVIDER_MAXIM
  {
    @NonNull
    public String getPackageName() { return "com.taxsee.taxsee"; }
    @NonNull
    public Utils.PartnerAppOpenMode getOpenMode() { return Utils.PartnerAppOpenMode.Direct; }
    public int getIcon() { return R.drawable.ic_taxi_logo_maksim; }
    public int getTitle() { return R.string.maxim_taxi_title; }
    @NonNull
    public String getProviderName() { return "Maxim"; }
  };

  @NonNull
  public abstract String getPackageName();
  @NonNull
  public abstract Utils.PartnerAppOpenMode getOpenMode();
  public abstract int getIcon();
  public abstract int getTitle();
  @NonNull
  public abstract String getProviderName();
}

package com.mopub.nativeads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.ads.AdDataAdapter;
import com.mapswithme.maps.ads.NetworkType;

class GoogleDataAdapter extends AdDataAdapter<GooglePlayServicesNative.GooglePlayServicesNativeAd>
{
  GoogleDataAdapter(@NonNull GooglePlayServicesNative.GooglePlayServicesNativeAd ad)
  {
    super(ad);
  }

  @Nullable
  @Override
  public String getTitle()
  {
    return getAd().getTitle();
  }

  @Nullable
  @Override
  public String getText()
  {
    return getAd().getText();
  }

  @Nullable
  @Override
  public String getIconImageUrl()
  {
    return getAd().getIconImageUrl();
  }

  @Nullable
  @Override
  public String getCallToAction()
  {
    return getAd().getCallToAction();
  }

  @Nullable
  @Override
  public String getPrivacyInfoUrl()
  {
    return null;
  }

  @NonNull
  @Override
  public NetworkType getType()
  {
    return NetworkType.MOPUB_GOOGLE;
  }
}

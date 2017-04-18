package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;
import android.widget.ImageView;

import com.facebook.ads.NativeAd;

class FacebookNativeAd extends CachedMwmNativeAd
{
  @NonNull
  private final NativeAd mAd;

  FacebookNativeAd(@NonNull NativeAd ad, long timestamp)
  {
    super(timestamp);
    mAd = ad;
  }

  FacebookNativeAd(@NonNull NativeAd ad)
  {
    super(0);
    mAd = ad;
  }

  @NonNull
  @Override
  public String getBannerId()
  {
    return mAd.getPlacementId();
  }

  @NonNull
  @Override
  public String getTitle()
  {
    return mAd.getAdTitle();
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return mAd.getAdBody();
  }

  @NonNull
  @Override
  public String getAction()
  {
    return mAd.getAdCallToAction();
  }

  @Override
  public void loadIcon(@NonNull View view)
  {
    NativeAd.downloadAndDisplayImage(mAd.getAdIcon(), (ImageView) view);
  }

  @Override
  public void unregister(@NonNull View view)
  {
    mAd.unregisterView();
  }

  @Override
  void register(@NonNull View view)
  {
    mAd.registerViewForInteraction(view);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.FACEBOOK;
  }

  @Nullable
  @Override
  public String getPrivacyInfoUrl()
  {
    return mAd.getAdChoicesLinkUrl();
  }
}

package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.view.View;
import android.widget.ImageView;

import com.my.target.ads.CustomParams;
import com.my.target.nativeads.NativeAd;
import com.my.target.nativeads.banners.NativePromoBanner;
import com.my.target.nativeads.models.ImageData;

import java.util.Map;

class MyTargetNativeAd extends CachedMwmNativeAd
{
  @NonNull
  private final NativeAd mAd;
  @NonNull
  private final String mBannerId;

  MyTargetNativeAd(@NonNull NativeAd ad, long timestamp)
  {
    super(timestamp);
    mAd = ad;
    CustomParams params = mAd.getCustomParams();
    Map<String, String> data = params.getData();
    mBannerId = data.get(MyTargetAdsLoader.ZONE_KEY_PARAMETER);
  }

  @NonNull
  @Override
  public String getBannerId()
  {
    return mBannerId;
  }

  @NonNull
  @Override
  public String getTitle()
  {
    return mAd.getBanner().getTitle();
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return mAd.getBanner().getDescription();
  }

  @NonNull
  @Override
  public String getAction()
  {
    return mAd.getBanner().getCtaText();
  }

  @Override
  public void loadIcon(@NonNull View view)
  {
    NativePromoBanner banner = mAd.getBanner();
    ImageData icon = banner.getIcon();
    if (icon != null)
      NativeAd.loadImageToView(icon, (ImageView) view);
  }

  @Override
  public void unregisterView()
  {
    mAd.unregisterView();
  }

  @Override
  void registerViewForInteraction(@NonNull View view)
  {
    mAd.registerView(view);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.MY_TARGET;
  }
}

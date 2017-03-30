package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.view.View;
import android.widget.ImageView;

import com.facebook.ads.NativeAd;
import com.mapswithme.maps.R;

import java.util.ArrayList;
import java.util.List;

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
  public void registerView(@NonNull View bannerView)
  {
    List<View> clickableViews = new ArrayList<>();
    clickableViews.add(bannerView.findViewById(R.id.tv__banner_title));
    clickableViews.add(bannerView.findViewById(R.id.tv__action_small));
    clickableViews.add(bannerView.findViewById(R.id.tv__action_large));
    mAd.registerViewForInteraction(bannerView, clickableViews);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.FACEBOOK;
  }
}

package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;

import com.mopub.nativeads.NativeAd;
import com.mopub.nativeads.NativeImageHelper;
import com.mopub.nativeads.StaticNativeAd;

class MopubNativeAd extends CachedMwmNativeAd
{
  @NonNull
  private final NativeAd mNativeAd;
  private final StaticNativeAd mAd;

  MopubNativeAd(@NonNull NativeAd ad, long timestamp)
  {
    super(timestamp);
    mNativeAd = ad;
    mAd = (StaticNativeAd) mNativeAd.getBaseNativeAd();
  }

  @NonNull
  @Override
  public String getBannerId()
  {
    return mNativeAd.getAdUnitId();
  }

  @NonNull
  @Override
  public String getTitle()
  {
    return TextUtils.isEmpty(mAd.getTitle()) ? "" : mAd.getTitle();
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return TextUtils.isEmpty(mAd.getText()) ? "" : mAd.getText();
  }

  @NonNull
  @Override
  public String getAction()
  {
    return TextUtils.isEmpty(mAd.getCallToAction()) ? "" : mAd.getCallToAction();
  }

  @Override
  public void loadIcon(@NonNull View view)
  {
    NativeImageHelper.loadImageView(mAd.getIconImageUrl(), (ImageView) view);
  }

  @Override
  public void unregister(@NonNull View view)
  {
    mNativeAd.clear(view);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.MOPUB;
  }

  @Override
  void register(@NonNull View view)
  {
    mNativeAd.prepare(view);
  }

  @Nullable
  @Override
  public String getPrivacyInfoUrl()
  {
    return mAd.getPrivacyInformationIconClickThroughUrl();
  }

  @Override
  void detachAdListener()
  {
    mNativeAd.setMoPubNativeEventListener(null);
  }

  @Override
  void attachAdListener(@NonNull Object listener)
  {
    if (!(listener instanceof NativeAd.MoPubNativeEventListener))
      throw new AssertionError("A listener for MoPub ad must be instance of " +
                               "NativeAd.MoPubNativeEventListener class! Not '"
                               + listener.getClass() + "'!");
    mNativeAd.setMoPubNativeEventListener((NativeAd.MoPubNativeEventListener) listener);
  }
}

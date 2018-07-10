package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;

import com.mopub.nativeads.AdData;
import com.mopub.nativeads.AdDelegate;
import com.mopub.nativeads.GoogleAdDelegate;
import com.mopub.nativeads.NativeAd;
import com.mopub.nativeads.NativeImageHelper;

class MopubNativeAd extends CachedMwmNativeAd
{
  @NonNull
  private final NativeAd mNativeAd;
  @NonNull
  private final AdData mAd;
  @Nullable
  private AdDelegate mAdDelegate;

  MopubNativeAd(@NonNull NativeAd ad, long timestamp)
  {
    super(timestamp);
    mNativeAd = ad;
    mAd = AdData.Make(mNativeAd.getBaseNativeAd());

    if (mAd.getAdType() == AdData.TYPE_GOOGLE)
      mAdDelegate = new GoogleAdDelegate();
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
  void register(@NonNull View view)
  {
    mNativeAd.prepare(view);
  }

  @Override
  public void unregister(@NonNull View view)
  {
    mNativeAd.clear(view);
  }

  @Override
  public void registerView(@NonNull View view)
  {
    super.registerView(view);

    if (mAdDelegate != null)
      mAdDelegate.registerView(mNativeAd.getBaseNativeAd(), view);
  }

  @Override
  public void unregisterView(@NonNull View view)
  {
    super.unregisterView(view);

    if (mAdDelegate != null)
      mAdDelegate.unregisterView(mNativeAd.getBaseNativeAd(), view);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.MOPUB;
  }

  @Nullable
  @Override
  public String getPrivacyInfoUrl()
  {
    return mAd.getPrivacyInfoUrl();
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

  @NonNull
  @Override
  public NetworkType getNetworkType()
  {
    return NetworkType.MOPUB;
  }
}

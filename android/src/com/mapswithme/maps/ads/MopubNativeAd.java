package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;

import com.mopub.nativeads.NativeAd;
import com.mopub.nativeads.NativeImageHelper;

public class MopubNativeAd extends CachedMwmNativeAd
{
  @NonNull
  private final NativeAd mNativeAd;
  @NonNull
  private final AdDataAdapter mDataAdapter;
  @Nullable
  private final AdRegistrator mAdRegistrator;

  public MopubNativeAd(@NonNull NativeAd ad, @NonNull AdDataAdapter adData,
                       @Nullable AdRegistrator registrator, long timestamp)
  {
    super(timestamp);
    mNativeAd = ad;
    mDataAdapter = adData;
    mAdRegistrator = registrator;
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
    return TextUtils.isEmpty(mDataAdapter.getTitle()) ? "" : mDataAdapter.getTitle();
  }

  @NonNull
  @Override
  public String getDescription()
  {
    return TextUtils.isEmpty(mDataAdapter.getText()) ? "" : mDataAdapter.getText();
  }

  @NonNull
  @Override
  public String getAction()
  {
    return TextUtils.isEmpty(mDataAdapter.getCallToAction()) ? "" : mDataAdapter.getCallToAction();
  }

  @Override
  public void loadIcon(@NonNull View view)
  {
    NativeImageHelper.loadImageView(mDataAdapter.getIconImageUrl(), (ImageView) view);
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

    if (mAdRegistrator != null)
      mAdRegistrator.registerView(mNativeAd.getBaseNativeAd(), view);
  }

  @Override
  public void unregisterView(@NonNull View view)
  {
    super.unregisterView(view);

    if (mAdRegistrator != null)
      mAdRegistrator.unregisterView(mNativeAd.getBaseNativeAd(), view);
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
    return mDataAdapter.getPrivacyInfoUrl();
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
    return mDataAdapter.getType();
  }

  @Override
  public String toString()
  {
    return super.toString() + ", mediated ad: " + mNativeAd.getBaseNativeAd();
  }
}

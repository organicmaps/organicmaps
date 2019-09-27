package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.my.target.common.CustomParams;
import com.my.target.nativeads.NativeAd;
import net.jcip.annotations.NotThreadSafe;

@NotThreadSafe
class MyTargetAdsLoader extends CachingNativeAdLoader implements NativeAd.NativeAdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = MyTargetAdsLoader.class.getSimpleName();
  private static final int SLOT = PrivateVariables.myTargetRbSlot();
  static final String ZONE_KEY_PARAMETER = "_SITEZONE";

  MyTargetAdsLoader(@Nullable OnAdCacheModifiedListener listener, @Nullable AdTracker tracker)
  {
    super(tracker, listener);
  }

  @Override
  void loadAdFromProvider(@NonNull Context context, @NonNull String bannerId)
  {
    NativeAd ad = new NativeAd(SLOT, context);
    ad.setListener(this);
    ad.getCustomParams().setCustomParam(ZONE_KEY_PARAMETER, bannerId);
    ad.load();
  }

  @Override
  public void onLoad(NativeAd nativeAd)
  {
    CachedMwmNativeAd ad = new MyTargetNativeAd(nativeAd, SystemClock.elapsedRealtime());
    onAdLoaded(ad.getBannerId(), ad);
  }

  @Override
  public void onNoAd(String s, NativeAd nativeAd)
  {
    LOGGER.w(TAG, "onNoAd s = " + s);
    CustomParams params = nativeAd.getCustomParams();
    String bannerId = params.getCustomParam(ZONE_KEY_PARAMETER);
    onError(bannerId, getProvider(), new MyTargetAdError(s));
  }

  @Override
  public void onClick(NativeAd nativeAd)
  {
    CustomParams params = nativeAd.getCustomParams();
    String bannerId = params.getData().get(ZONE_KEY_PARAMETER);
    onAdClicked(bannerId);
  }

  @Override
  public void onShow(NativeAd nativeAd)
  {
    // No op.
  }

  @Override
  public void onVideoPlay(@NonNull NativeAd nativeAd)
  {
    /* Do nothing */
  }

  @Override
  public void onVideoPause(@NonNull NativeAd nativeAd)
  {
    /* Do nothing */
  }

  @Override
  public void onVideoComplete(@NonNull NativeAd nativeAd)
  {
    /* Do nothing */
  }

  @NonNull
  @Override
  String getProvider()
  {
    return Providers.MY_TARGET;
  }
}

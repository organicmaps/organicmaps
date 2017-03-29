package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.my.target.ads.CustomParams;
import com.my.target.nativeads.NativeAd;
import net.jcip.annotations.NotThreadSafe;

@NotThreadSafe
class MyTargetAdsLoader extends CachingNativeAdLoader implements NativeAd.NativeAdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = MyTargetAdsLoader.class.getSimpleName();
  //FIXME: read correct slot from private.h
  private static final int SLOT = 93418;
  private static final String ZONE_KEY_PARAMETER = "_SITEZONE";

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
    CustomParams params = nativeAd.getCustomParams();
    String bannerId = params.getData().get(ZONE_KEY_PARAMETER);
    onAdLoaded(bannerId, new MyTargetNativeAd(nativeAd, SystemClock.elapsedRealtime()));
  }

  @Override
  public void onNoAd(String s, NativeAd nativeAd)
  {
    LOGGER.w(TAG, "onNoAd s = " + s);
    CustomParams params = nativeAd.getCustomParams();
    String bannerId = params.getData().get(ZONE_KEY_PARAMETER);
    onError(bannerId, new MyTargetNativeAd(nativeAd, 0), new MyTargetAdError(s));
  }

  @Override
  public void onClick(NativeAd nativeAd)
  {
    CustomParams params = nativeAd.getCustomParams();
    String bannerId = params.getData().get(ZONE_KEY_PARAMETER);
    onAdClicked(bannerId);
  }

  @NonNull
  @Override
  public String getProvider()
  {
    return Providers.MY_TARGET;
  }
}

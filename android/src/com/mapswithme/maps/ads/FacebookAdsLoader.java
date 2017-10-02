package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.facebook.ads.Ad;
import com.facebook.ads.AdError;
import com.facebook.ads.AdListener;
import com.facebook.ads.NativeAd;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import net.jcip.annotations.NotThreadSafe;

import java.util.EnumSet;

@NotThreadSafe
class FacebookAdsLoader extends CachingNativeAdLoader implements AdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = FacebookAdsLoader.class.getSimpleName();

  FacebookAdsLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                    @Nullable AdTracker tracker)
  {
    super(tracker, cacheListener);
  }

  @Override
  public void onError(Ad ad, AdError adError)
  {
    LOGGER.w(TAG, "A error '" + adError.getErrorMessage() + "' is occurred while loading " +
                  "an ad for banner id '" + ad.getPlacementId() + "'");

    onError(ad.getPlacementId(), getProvider(), new FacebookAdError(adError));
  }

  @Override
  public void onAdLoaded(Ad ad)
  {
    CachedMwmNativeAd nativeAd = new FacebookNativeAd((NativeAd) ad, SystemClock.elapsedRealtime());
    onAdLoaded(nativeAd.getBannerId(), nativeAd);
  }

  @Override
  public void onAdClicked(Ad ad)
  {
    onAdClicked(ad.getPlacementId());
  }

  @Override
  public void onLoggingImpression(Ad ad)
  {
    LOGGER.i(TAG, "onLoggingImpression");
  }

  @Override
  void loadAdFromProvider(@NonNull Context context, @NonNull String bannerId)
  {
    NativeAd ad = new NativeAd(context, bannerId);
    ad.setAdListener(this);
    LOGGER.d(TAG, "Loading is started");
    ad.loadAd(EnumSet.of(NativeAd.MediaCacheFlag.ICON));
  }

  @NonNull
  @Override
  String getProvider()
  {
    return Providers.FACEBOOK;
  }
}

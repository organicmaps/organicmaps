package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;

import com.facebook.ads.Ad;
import com.facebook.ads.AdError;
import com.facebook.ads.AdListener;
import com.facebook.ads.NativeAd;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import net.jcip.annotations.NotThreadSafe;

import java.util.EnumSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

@NotThreadSafe
class FacebookAdsLoader extends BaseNativeAdLoader implements AdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = FacebookAdsLoader.class.getSimpleName();
  private static final long REQUEST_INTERVAL_MS = 30 * 1000;
  private static final Map<String, FacebookNativeAd> CACHE = new HashMap<>();
  private static final Set<String> PENDING_REQUESTS = new HashSet<>();
  @Nullable
  private OnAdCacheModifiedListener mCacheListener;
  @Nullable
  private AdTracker mTracker;

  FacebookAdsLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                           @Nullable AdTracker tracker)
  {
    mCacheListener = cacheListener;
    mTracker = tracker;
  }

  /**
   * Loads an ad for a specified banner id. If there is a cached ad, the caller will be notified
   * immediately through {@link NativeAdListener#onAdLoaded(MwmNativeAd)}.
   * Otherwise, the caller will be notified once an ad is loaded through the mentioned method.
   *
   * <br><br><b>Important note: </b> if there is a cached ad for the requested banner id, and that ad
   * has a good impression indicator, and there is at least {@link #REQUEST_INTERVAL_MS} between the
   * first time that ad was requested and the current time the new ad will be loaded.
   *
   */
  @UiThread
  public void loadAd(@NonNull Context context, @NonNull String bannerId)
  {
    LOGGER.d(TAG, "Load a facebook ad for a banner id '" + bannerId + "'");

    FacebookNativeAd cachedAd = getAdByIdFromCache(bannerId);

    if (cachedAd == null)
    {
      LOGGER.d(TAG, "There is no an ad in a cache");
      loadAdInternal(context, bannerId);
      return;
    }

    if (mTracker != null && mTracker.isImpressionGood(bannerId)
        && SystemClock.elapsedRealtime() - cachedAd.getLoadedTime() >= REQUEST_INTERVAL_MS)
    {
      LOGGER.d(TAG, "A new ad will be loaded because the previous one has a good impression");
      loadAdInternal(context, bannerId);
    }

    if (getAdListener() != null)
      getAdListener().onAdLoaded(cachedAd);
  }

  /**
   * Indicates whether the ad is loading right now or not.
   *
   * @param bannerId A banner id that an ad is loading for.
   * @return true if an ad is loading, otherwise - false.
   */
  public boolean isAdLoading(@NonNull String bannerId)
  {
    return PENDING_REQUESTS.contains(bannerId);
  }

  @Nullable
  private FacebookNativeAd getAdByIdFromCache(@NonNull String bannerId)
  {
    return CACHE.get(bannerId);
  }

  private boolean isCacheEmptyForId(@NonNull String bannerId)
  {
    return getAdByIdFromCache(bannerId) == null;
  }

  @Override
  public void onError(Ad ad, AdError adError)
  {
    LOGGER.w(TAG, "A error '" + adError.getErrorMessage() + "' is occurred while loading " +
                  "an ad for banner id '" + ad.getPlacementId() + "'");
    PENDING_REQUESTS.remove(ad.getPlacementId());
    if (getAdListener() != null)
      getAdListener().onError(new FacebookNativeAd((NativeAd)ad), new FacebookAdError(adError));
  }

  @Override
  public void onAdLoaded(Ad ad)
  {
    LOGGER.d(TAG, "An ad for id '" + ad.getPlacementId() + "' is loaded");
    PENDING_REQUESTS.remove(ad.getPlacementId());

    boolean isCacheWasEmpty = isCacheEmptyForId(ad.getPlacementId());

    LOGGER.d(TAG, "Put a facebook ad to cache");
    MwmNativeAd nativeAd = new FacebookNativeAd((NativeAd) ad, SystemClock.elapsedRealtime());
    putInCache(ad.getPlacementId(), (FacebookNativeAd) nativeAd);

    if (isCacheWasEmpty && getAdListener() != null)
      getAdListener().onAdLoaded(nativeAd);
  }

  @Override
  public void onAdClicked(Ad ad)
  {
    if (getAdListener() != null)
    {
      MwmNativeAd nativeAd = getAdByIdFromCache(ad.getPlacementId());
      if (nativeAd == null)
        throw new AssertionError("A facebook native ad must be presented in a cache when it's clicked!");

      getAdListener().onClick(nativeAd);
    }
  }

  private void loadAdInternal(@NonNull Context context, @NonNull String bannerId)
  {
    if (PENDING_REQUESTS.contains(bannerId))
    {
      LOGGER.d(TAG, "The ad request for banner id '" + bannerId + "' hasn't been completed yet.");
      return;
    }

    NativeAd ad = new NativeAd(context, bannerId);
    ad.setAdListener(this);
    LOGGER.d(TAG, "Loading is started");
    ad.loadAd(EnumSet.of(NativeAd.MediaCacheFlag.ICON));
    PENDING_REQUESTS.add(bannerId);
  }

  private void putInCache(@NonNull String key, @NonNull FacebookNativeAd value)
  {
    CACHE.put(key, value);
    if (mCacheListener != null)
      mCacheListener.onPut(key);
  }

  private void removeFromCache(@NonNull String key, @NonNull FacebookNativeAd value)
  {
    CACHE.remove(key);
    if (mCacheListener != null)
      mCacheListener.onRemoved(key);
  }
}

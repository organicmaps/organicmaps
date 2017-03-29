package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

abstract class CachedNativeAdLoader extends BaseNativeAdLoader
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = CachedNativeAdLoader.class.getSimpleName();
  private static final long REQUEST_INTERVAL_MS = 30 * 1000;
  private static final Map<String, CachedMwmNativeAd> CACHE = new HashMap<>();
  private static final Set<String> PENDING_REQUESTS = new HashSet<>();

  @Nullable
  private AdTracker mTracker;
  @Nullable
  private OnAdCacheModifiedListener mCacheListener;

  CachedNativeAdLoader(@Nullable AdTracker tracker, @Nullable OnAdCacheModifiedListener listener)
  {
    mTracker = tracker;
    mCacheListener = listener;
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
  @Override
  public void loadAd(@NonNull Context context, @NonNull String bannerId)
  {
    LOGGER.d(TAG, "Load the ad for a banner id '" + bannerId + "'");

    if (isAdLoading(bannerId))
    {
      LOGGER.d(TAG, "The ad request for banner id '" + bannerId + "' hasn't been completed yet.");
      return;
    }

    CachedMwmNativeAd cachedAd = getAdByIdFromCache(bannerId);

    if (cachedAd == null)
    {
      LOGGER.d(TAG, "There is no an ad in a cache");
      requestAd(context, bannerId);
      return;
    }

    if (mTracker != null && mTracker.isImpressionGood(bannerId)
        && SystemClock.elapsedRealtime() - cachedAd.getLoadedTime() >= REQUEST_INTERVAL_MS)
    {
      LOGGER.d(TAG, "A new ad will be loaded because the previous one has a good impression");
      requestAd(context, bannerId);
    }

    if (getAdListener() != null)
      getAdListener().onAdLoaded(cachedAd);
  }

  private void requestAd(@NonNull Context context, @NonNull String bannerId)
  {
    requestAdForBannerId(context, bannerId);
    PENDING_REQUESTS.add(bannerId);
  }

  abstract void requestAdForBannerId(@NonNull Context context, @NonNull String bannerId);

  void onError(@NonNull String bannerId)
  {
    PENDING_REQUESTS.remove(bannerId);
  }

  void onAdLoaded(@NonNull String bannerId, @NonNull CachedMwmNativeAd ad)
  {
    LOGGER.d(TAG, "An ad for id '" + bannerId + "' is loaded");
    PENDING_REQUESTS.remove(bannerId);

    boolean isCacheWasEmpty = isCacheEmptyForId(bannerId);

    LOGGER.d(TAG, "Put the ad to cache");
    putInCache(bannerId, ad);

    if (isCacheWasEmpty && getAdListener() != null)
      getAdListener().onAdLoaded(ad);
  }

  void onAdClicked(@NonNull String bannerId)
  {
    if (getAdListener() != null)
    {
      MwmNativeAd nativeAd = getAdByIdFromCache(bannerId);
      if (nativeAd == null)
        throw new AssertionError("A facebook native ad must be presented in a cache when it's clicked!");

      getAdListener().onClick(nativeAd);
    }
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

  private void putInCache(@NonNull String key, @NonNull CachedMwmNativeAd value)
  {
    CACHE.put(key, value);
    if (mCacheListener != null)
      mCacheListener.onPut(key);
  }

  private void removeFromCache(@NonNull String key, @NonNull CachedMwmNativeAd value)
  {
    CACHE.remove(key);
    if (mCacheListener != null)
      mCacheListener.onRemoved(key);
  }

  @Nullable
  private CachedMwmNativeAd getAdByIdFromCache(@NonNull String bannerId)
  {
    return CACHE.get(bannerId);
  }

  private boolean isCacheEmptyForId(@NonNull String bannerId)
  {
    return getAdByIdFromCache(bannerId) == null;
  }
}

package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import net.jcip.annotations.NotThreadSafe;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

@NotThreadSafe
abstract class CachingNativeAdLoader extends BaseNativeAdLoader
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = CachingNativeAdLoader.class.getSimpleName();
  private static final long REQUEST_INTERVAL_MS = 5 * 1000;
  private static final Map<BannerKey, CachedMwmNativeAd> CACHE = new HashMap<>();
  private static final Set<BannerKey> PENDING_REQUESTS = new HashSet<>();

  @Nullable
  private final AdTracker mTracker;
  @Nullable
  private final OnAdCacheModifiedListener mCacheListener;

  CachingNativeAdLoader(@Nullable AdTracker tracker, @Nullable OnAdCacheModifiedListener listener)
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
  @CallSuper
  public void loadAd(@NonNull Context context, @NonNull String bannerId)
  {
    LOGGER.d(TAG, "Load the ad for a banner id '" + bannerId + "'");
    final BannerKey key = new BannerKey(getProvider(), bannerId);
    CachedMwmNativeAd cachedAd = getAdByIdFromCache(key);

    if (cachedAd == null)
    {
      LOGGER.d(TAG, "There is no an ad in a cache");
      loadAdInternally(context, bannerId);
      return;
    }

    if (isImpressionGood(cachedAd) && canBeReloaded(cachedAd))
    {
      LOGGER.d(TAG, "A new ad will be loaded because the previous one has a good impression");
      loadAdInternally(context, bannerId);
    }

    if (getAdListener() != null)
    {
      LOGGER.d(TAG, "A cached ad '" + cachedAd.getTitle() + "' is set immediately");
      getAdListener().onAdLoaded(cachedAd);
    }
  }

  @CallSuper
  @Override
  public void cancel()
  {
    super.cancel();
    PENDING_REQUESTS.clear();
  }

  private boolean isImpressionGood(@NonNull CachedMwmNativeAd ad)
  {
    return mTracker != null && mTracker.isImpressionGood(ad.getProvider(), ad.getBannerId());
  }

  private static boolean canBeReloaded(@NonNull CachedMwmNativeAd ad)
  {
    return SystemClock.elapsedRealtime() - ad.getLoadedTime() >= REQUEST_INTERVAL_MS;
  }

  private void loadAdInternally(@NonNull Context context, @NonNull String bannerId)
  {
    if (isAdLoading(bannerId))
    {
      LOGGER.d(TAG, "The ad request for banner id '" + bannerId + "' hasn't been completed yet.");
      return;
    }

    loadAdFromProvider(context, bannerId);
    PENDING_REQUESTS.add(new BannerKey(getProvider(), bannerId));
  }

  abstract void loadAdFromProvider(@NonNull Context context, @NonNull String bannerId);

  /**
   * Returns a provider name for this ad.
   */
  @NonNull
  abstract String getProvider();

  void onError(@NonNull String bannerId, @NonNull String provider, @NonNull NativeAdError error)
  {
    PENDING_REQUESTS.remove(new BannerKey(getProvider(), bannerId));
    if (getAdListener() != null)
      getAdListener().onError(bannerId, provider, error);
  }

  void onAdLoaded(@NonNull String bannerId, @NonNull CachedMwmNativeAd ad)
  {
    BannerKey key = new BannerKey(getProvider(), bannerId);
    LOGGER.d(TAG, "A new ad for id '" + key + "' is loaded, title = " + ad.getTitle());
    PENDING_REQUESTS.remove(key);

    boolean isCacheWasEmpty = isCacheEmptyForId(key);

    LOGGER.d(TAG, "Put the ad '" + ad.getTitle() + "' to cache, isCacheWasEmpty = " + isCacheWasEmpty);
    putInCache(key, ad);

    if (isCacheWasEmpty && getAdListener() != null)
      getAdListener().onAdLoaded(ad);
  }

  void onAdClicked(@NonNull String bannerId)
  {
    if (getAdListener() != null)
    {
      MwmNativeAd nativeAd = getAdByIdFromCache(new BannerKey(getProvider(), bannerId));
      if (nativeAd == null)
        throw new AssertionError("A native ad must be presented in a cache when it's clicked!");

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
    return PENDING_REQUESTS.contains(new BannerKey(getProvider(), bannerId));
  }

  private void putInCache(@NonNull BannerKey key, @NonNull CachedMwmNativeAd value)
  {
    CACHE.put(key, value);
    if (mCacheListener != null)
      mCacheListener.onPut(key);
  }

  private void removeFromCache(@NonNull BannerKey key, @NonNull CachedMwmNativeAd value)
  {
    CACHE.remove(key);
    if (mCacheListener != null)
      mCacheListener.onRemoved(key);
  }

  @Nullable
  private CachedMwmNativeAd getAdByIdFromCache(@NonNull BannerKey key)
  {
    return CACHE.get(key);
  }

  private boolean isCacheEmptyForId(@NonNull BannerKey key)
  {
    return getAdByIdFromCache(key) == null;
  }

  @CallSuper
  @Override
  public void detach()
  {
    for (CachedMwmNativeAd ad : CACHE.values())
      ad.detachAdListener();
  }

  @CallSuper
  @Override
  public void attach()
  {
    for (CachedMwmNativeAd ad : CACHE.values())
    {
      if (ad.getProvider().equals(getProvider()))
        ad.attachAdListener(this);
    }
  }
}

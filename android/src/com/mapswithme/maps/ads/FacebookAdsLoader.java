package com.mapswithme.maps.ads;

import android.content.Context;
import android.os.SystemClock;
import android.support.annotation.CallSuper;
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
public class FacebookAdsLoader
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = FacebookAdsLoader.class.getSimpleName();
  private static final long EXPIRATION_TIME_MS = 60 * 60 * 1000;
  private static final Map<String, FacebookAd> CACHE = new HashMap<>();
  private static final Set<String> PENDING_REQUESTS = new HashSet<>();

  /**
   * Loads an ad for a specified placement id. If there is a cached ad, and it's not expired,
   * that ad will be returned immediately. Otherwise, this method returns null, and a listener will
   * be notified when a requested ad is loaded.
   *
   * @param context An activity context.
   * @param placementId A placement id that ad will be loaded for.
   * @param listener A listener to be notified when an ad is loaded.
   * @return A cached banner if it presents, otherwise <code>null</code>
   */
  @Nullable
  @UiThread
  public NativeAd load(@NonNull Context context, @NonNull String placementId,
                       @NonNull FacebookAdsListener listener)
  {
    LOGGER.d(TAG, "Load a facebook ad for a placement id '" + placementId + "'");
    if (!PENDING_REQUESTS.contains(placementId))
    {
      NativeAd ad = new NativeAd(context, placementId);
      ad.setAdListener(listener);
      ad.loadAd(EnumSet.of(NativeAd.MediaCacheFlag.ICON));
      LOGGER.d(TAG, "Loading is started");
      PENDING_REQUESTS.add(placementId);
    } else
    {
      LOGGER.d(TAG, "The ad request for placement id '" + placementId + "' hasn't been completed yet.");
    }

    FacebookAd cachedAd = CACHE.get(placementId);

    if (cachedAd == null)
    {
      LOGGER.d(TAG, "There is no an ad in a cache for a placement id '" + placementId + "'");
      return null;
    }

    long cacheTime = SystemClock.elapsedRealtime() - cachedAd.getLoadedTime();
    if (cacheTime >= EXPIRATION_TIME_MS)
    {
      LOGGER.d(TAG, "A facebook ad is expired for a placement id '" + placementId + "'. " +
                    "An expiration time is " + (cacheTime - EXPIRATION_TIME_MS) + " ms");
      CACHE.remove(placementId);
      return null;
    }

    return cachedAd.getAd();
  }

  /**
   * Indicates whether the ad is loading right now or not.
   *
   * @param placementId A placement id that an ad is loading for.
   * @return true if an ad is loading, otherwise - false.
   */
  public static boolean isAdLoadingForId(@NonNull String placementId)
  {
    return PENDING_REQUESTS.contains(placementId);
  }

  /**
   * Retrieves the cached ad for the specified placement id.
   *
   * @return A cached ad or <code>null</code> if it doesn't exist.
   */
  @Nullable
  public static NativeAd getAdByIdFromCache(@NonNull String placementId)
  {
    FacebookAd ad = CACHE.get(placementId);
    return ad != null ? ad.getAd() : null;
  }

  /**
   * Indicates whether there is a cached ad for the specified placement id or not.
   *
   * @return <code>true</code> if there is a cached ad, otherwise - <code>false</code>
   */
  public static boolean isCacheEmptyForId(@NonNull String placementId)
  {
    return getAdByIdFromCache(placementId) == null;
  }

  public abstract static class FacebookAdsListener implements AdListener
  {
    @Override
    public final void onAdLoaded(Ad ad)
    {
      LOGGER.d(TAG, "An ad for id '" + ad.getPlacementId() + "' is loaded");
      PENDING_REQUESTS.remove(ad.getPlacementId());

      boolean isCacheWasEmpty = isCacheEmptyForId(ad.getPlacementId());

      LOGGER.d(TAG, "Put a facebook ad to cache for a placement id '" + ad.getPlacementId());
      CACHE.put(ad.getPlacementId(), new FacebookAd((NativeAd)ad, SystemClock.elapsedRealtime()));

      if (isCacheWasEmpty)
        onFacebookAdLoaded((NativeAd) ad);
    }

    @Override
    @CallSuper
    public final void onError(Ad ad, AdError adError)
    {
      LOGGER.w(TAG, "A error '" + adError + "' is occurred while loading an ad for placement id " +
                    "'" + ad.getPlacementId() + "'");
      PENDING_REQUESTS.remove(ad.getPlacementId());
      onError(ad, adError, isCacheEmptyForId(ad.getPlacementId()));
    }

    /**
     * Called <b>only if</b> there is no a cached ad for a corresponding placement id
     * contained within a downloaded ad object, i.e. {@link NativeAd#getPlacementId()}.
     * In other words, this callback will never be called until there is a cached ad for
     * a requested placement id.
     *
     * @param ad A downloaded ad.
     */
    @UiThread
    protected abstract void onFacebookAdLoaded(@NonNull NativeAd ad);

    /**
     * Notifies about a error occurred while loading the ad.
     *
     * @param isCacheEmpty Will be true if there is a cached ad for the same placement id,
     *                     otherwise - false
     */
    @UiThread
    protected abstract void onError(@NonNull Ad ad, @NonNull AdError adError, boolean isCacheEmpty);
  }

  private static class FacebookAd
  {
    @NonNull
    private NativeAd mAd;
    private final long mLoadedTime;

    FacebookAd(@NonNull NativeAd ad, long timestamp)
    {
      mLoadedTime = timestamp;
      mAd = ad;
    }

    long getLoadedTime()
    {
      return mLoadedTime;
    }

    @NonNull
    NativeAd getAd()
    {
      return mAd;
    }
  }
}

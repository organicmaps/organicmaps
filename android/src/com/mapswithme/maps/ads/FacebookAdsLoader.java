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
public class FacebookAdsLoader implements AdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = FacebookAdsLoader.class.getSimpleName();
  private static final long REQUEST_INTERVAL_MS = 30 * 1000;
  private final Map<String, FacebookAd> mCache = new HashMap<>();
  private final Set<String> mPendingRequests = new HashSet<>();
  @Nullable
  private FacebookAdsListener mAdsListener;
  @Nullable
  private OnAdCacheModifiedListener mCacheListener;

  /**
   * Loads an ad for a specified placement id. If there is a cached ad, and it's not expired,
   * that ad will be returned immediately. Otherwise, this method returns null, and {@link #mAdsListener} will
   * be notified when the requested ad is loaded.
   *
   * @param context An activity context.
   * @param placementId A placement id that ad will be loaded for.
   * @return A cached banner if it presents, otherwise <code>null</code>
   */
  @Nullable
  @UiThread
  public NativeAd load(@NonNull Context context, @NonNull String placementId)
  {
    LOGGER.d(TAG, "Load a facebook ad for a placement id '" + placementId + "'");

    FacebookAd cachedAd = mCache.get(placementId);

    if (cachedAd == null)
    {
      LOGGER.d(TAG, "There is no an ad in a cache");
      loadAdInternal(context, placementId);
      return null;
    }

    if (/** Tracker.checkShowTime(placmenetId)  &&**/
        SystemClock.elapsedRealtime() - cachedAd.getLoadedTime() >= REQUEST_INTERVAL_MS)
    {
      LOGGER.d(TAG, "Ad should be reloaded");
      loadAdInternal(context, placementId);
    }

    return cachedAd.getAd();
  }

  /**
   * Indicates whether the ad is loading right now or not.
   *
   * @param placementId A placement id that an ad is loading for.
   * @return true if an ad is loading, otherwise - false.
   */
  public boolean isAdLoadingForId(@NonNull String placementId)
  {
    return mPendingRequests.contains(placementId);
  }

  /**
   * Retrieves the cached ad for the specified placement id.
   *
   * @return A cached ad or <code>null</code> if it doesn't exist.
   */
  @Nullable
  public NativeAd getAdByIdFromCache(@NonNull String placementId)
  {
    FacebookAd ad = mCache.get(placementId);
    return ad != null ? ad.getAd() : null;
  }

  /**
   * Indicates whether there is a cached ad for the specified placement id or not.
   *
   * @return <code>true</code> if there is a cached ad, otherwise - <code>false</code>
   */
  public boolean isCacheEmptyForId(@NonNull String placementId)
  {
    return getAdByIdFromCache(placementId) == null;
  }

  @Override
  public void onError(Ad ad, AdError adError)
  {
    LOGGER.w(TAG, "A error '" + adError + "' is occurred while loading an ad for placement id " +
                  "'" + ad.getPlacementId() + "'");
    mPendingRequests.remove(ad.getPlacementId());
    if (mAdsListener != null)
      mAdsListener.onError(ad, adError, isCacheEmptyForId(ad.getPlacementId()));
  }

  @Override
  public void onAdLoaded(Ad ad)
  {
    LOGGER.d(TAG, "An ad for id '" + ad.getPlacementId() + "' is loaded");
    mPendingRequests.remove(ad.getPlacementId());

    boolean isCacheWasEmpty = isCacheEmptyForId(ad.getPlacementId());

    LOGGER.d(TAG, "Put a facebook ad to cache");
    putInCache(ad.getPlacementId(), new FacebookAd((NativeAd)ad, SystemClock.elapsedRealtime()));

    if (isCacheWasEmpty && mAdsListener != null)
      mAdsListener.onFacebookAdLoaded((NativeAd) ad);
  }

  @Override
  public void onAdClicked(Ad ad)
  {
    if (mAdsListener != null)
      mAdsListener.onAdClicked(ad);
  }

  public void setAdsListener(@Nullable FacebookAdsListener adsListener)
  {
    mAdsListener = adsListener;
  }

  public void setCacheListener(@Nullable OnAdCacheModifiedListener cacheListener)
  {
    mCacheListener = cacheListener;
  }

  private void loadAdInternal(@NonNull Context context, @NonNull String placementId)
  {
    if (mPendingRequests.contains(placementId))
    {
      LOGGER.d(TAG, "The ad request for placement id '" + placementId + "' hasn't been completed yet.");
      return;
    }

    NativeAd ad = new NativeAd(context, placementId);
    ad.setAdListener(this);
    LOGGER.d(TAG, "Loading is started");
    ad.loadAd(EnumSet.of(NativeAd.MediaCacheFlag.ICON));
    mPendingRequests.add(placementId);
  }

  private void putInCache(@NonNull String key, @NonNull FacebookAd value)
  {
    mCache.put(key, value);
    if (mCacheListener != null)
      mCacheListener.onPut(key);
  }

  private void removeFromCache(@NonNull String key, @NonNull FacebookAd value)
  {
    mCache.remove(key);
    if (mCacheListener != null)
      mCacheListener.onRemove(key);
  }

  public interface FacebookAdsListener
  {
    /**
     * Called <b>only if</b> there is no a cached ad for a corresponding placement id
     * contained within a downloaded ad object, i.e. {@link NativeAd#getPlacementId()}.
     * In other words, this callback will never be called until there is a cached ad for
     * a requested placement id.
     *
     * @param ad A downloaded ad.
     */
    @UiThread
    void onFacebookAdLoaded(@NonNull NativeAd ad);

    /**
     * Notifies about a error occurred while loading the ad.
     *
     * @param isCacheEmpty Will be true if there is a cached ad for the same placement id,
     *                     otherwise - false
     */
    @UiThread
    void onError(@NonNull Ad ad, @NonNull AdError adError, boolean isCacheEmpty);

    @UiThread
    void onAdClicked(@NonNull Ad ad);
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

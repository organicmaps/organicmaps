package com.mapswithme.maps.ads;

import android.annotation.SuppressLint;
import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Represents a native loader that provides interface to load a few banners simultaneously, i.e.
 * concurrently. This loader makes a decision about which native ad should be posted to the listener
 * based on obtained results. If all native ads for requested banners are obtained, the native ad
 * with the highest priority will be post. Now, MyTarget banner has a high priority. If there is no
 * MyTarget banner in the requested banner list, the first obtained native ad will be posted
 * immediately. Otherwise, this loader will try to obtain/wait for MyTarget native ad even if another
 * provider already give theirs ads.
 *
 */
public class CompoundNativeAdLoader extends BaseNativeAdLoader implements NativeAdListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = CompoundNativeAdLoader.class.getSimpleName();
  private static final int TIMEOUT_MS = 5000;
  @NonNull
  private final List<NativeAdLoader> mLoaders = new ArrayList<>();
  @Nullable
  private final OnAdCacheModifiedListener mCacheListener;
  @Nullable
  private final AdTracker mAdTracker;
  @NonNull
  private final Set<String> mFailedProviders = new HashSet<>();
  @Nullable
  private Runnable mDelayedNotification;
  /**
   * Indicates about whether the composite loading can be considered as completed or not.
   */
  private boolean mLoadingCompleted;

  CompoundNativeAdLoader(@Nullable OnAdCacheModifiedListener cacheListener,
                                   @Nullable AdTracker adTracker)
  {
    mCacheListener = cacheListener;
    mAdTracker = adTracker;
  }

  @androidx.annotation.UiThread
  public void loadAd(@NonNull Context context, @NonNull List<Banner> banners)
  {
    LOGGER.i(TAG, "Load ads for " + banners);
    detach();
    cancel();
    mLoadingCompleted = false;
    mFailedProviders.clear();

    if (banners.size() == 0)
      return;

    for (Banner banner : banners)
    {
      if (TextUtils.isEmpty(banner.getId()))
        throw new AssertionError("A banner id mustn't be empty!");

      NativeAdLoader loader = Factory.createLoaderForBanner(banner, mCacheListener, mAdTracker);
      mLoaders.add(loader);
      attach();
      loader.setAdListener(this);
      // TODO: this workaround is need to avoid memory leak of activity context in MyTarget SDK.
      // The fix of myTarged sdk will be done in this issue https://jira.mail.ru/browse/MOBADS-207.
      // After the mentioned issued is fixed, this workaround should be removed. Also, we can't use
      // the application context for all providers, because some of them (e.g. Mopub) requires an
      // activity context and can't work with application context correctly.
      if (loader instanceof MyTargetAdsLoader)
        loader.loadAd(context.getApplicationContext(), banner.getId());
      else
        loader.loadAd(context, banner.getId());
    }
  }

  @Override
  public void loadAd(@NonNull Context context, @NonNull String bannerId)
  {
    throw new UnsupportedOperationException("A compound loader doesn't support this operation!");
  }

  @Override
  public boolean isAdLoading(@NonNull String bannerId)
  {
    throw new UnsupportedOperationException("A compound loader doesn't support this operation!");
  }

  @SuppressLint("MissingSuperCall")
  // Don't need to call super here, because we don't need to null the mAdListener from the
  // CompoundNativeAdLoader
  @Override
  public void cancel()
  {
    for (NativeAdLoader loader : mLoaders)
      loader.cancel();
    mLoaders.clear();
  }

  @Override
  public void detach()
  {
    for (NativeAdLoader loader : mLoaders)
      loader.detach();
  }

  @Override
  public void attach()
  {
    for (NativeAdLoader loader : mLoaders)
      loader.attach();
  }

  public boolean isAdLoading()
  {
    return !mLoadingCompleted;
  }

  @Override
  public void onAdLoaded(@NonNull MwmNativeAd ad)
  {
    if (mDelayedNotification != null)
    {
      UiThread.cancelDelayedTasks(mDelayedNotification);
      mDelayedNotification = null;
    }

    if (mLoadingCompleted)
      return;

    // If only one banner is requested and obtained, it will be posted immediately to the listener.
    if (mLoaders.size() == 1)
    {
      onAdLoadingCompleted(ad);
      return;
    }

    String provider = ad.getProvider();
    // MyTarget ad has the highest priority, so we notify the listener as soon as that ad is obtained.
    if (Providers.MY_TARGET.equals(provider))
    {
      onAdLoadingCompleted(ad);
      return;
    }

    // If MyTarget ad is failed, the ad from another provider should be posted to the listener.
    if (mFailedProviders.contains(Providers.MY_TARGET))
    {
      onAdLoadingCompleted(ad);
      return;
    }

    // Otherwise, we must wait a TIMEOUT_MS for the high priority ad.
    // If the high priority ad is not obtained in TIMEOUT_MS, the last obtained ad will be posted
    // to the listener.
    mDelayedNotification = new DelayedNotification(ad);
    UiThread.runLater(mDelayedNotification, TIMEOUT_MS);
  }

  @Override
  public void onError(@NonNull String bannerId, @NonNull String provider, @NonNull NativeAdError error)
  {
    mFailedProviders.add(provider);

    // If all providers give nothing, the listener will be notified about the error.
    if (mFailedProviders.size() == mLoaders.size())
    {
      if (getAdListener() != null)
        getAdListener().onError(bannerId, provider, error);
      return;
    }

    // If the high priority ad is just failed, the timer should be forced if it's started.
    if (Providers.MY_TARGET.equals(provider) && mDelayedNotification != null)
    {
      mDelayedNotification.run();
      UiThread.cancelDelayedTasks(mDelayedNotification);
      mDelayedNotification = null;
      mLoadingCompleted = true;
    }
  }

  @Override
  public void onClick(@NonNull MwmNativeAd ad)
  {
    if (getAdListener() != null)
      getAdListener().onClick(ad);
  }

  private void onAdLoadingCompleted(@NonNull MwmNativeAd ad)
  {
    if (getAdListener() != null)
      getAdListener().onAdLoaded(ad);
    mLoadingCompleted = true;
  }

  private class DelayedNotification implements Runnable
  {
    @NonNull
    private final MwmNativeAd mAd;

    private DelayedNotification(@NonNull MwmNativeAd ad)
    {
      mAd = ad;
    }

    @Override
    public void run()
    {
      onAdLoadingCompleted(mAd);
    }
  }
}
